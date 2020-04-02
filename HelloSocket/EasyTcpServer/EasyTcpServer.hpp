#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

#ifdef _WIN32
	#define FD_SETSIZE      25060
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#include<windows.h>
	#include<WinSock2.h>
	#pragma comment(lib,"ws2_32.lib")
#else
	#include<unistd.h> //uni std
	#include<arpa/inet.h>
	#include<string.h>

	#define SOCKET int
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
#endif

#include<stdio.h>
#include<vector>
#include <map>
#include<thread>
#include<mutex>
#include<atomic>  // 原子操作库

#include"MessageHeader.hpp"
#include"CELLTimestamp.hpp"  // c++11定时库

//缓冲区最小单元大小
#ifndef RECV_BUFF_SZIE
#define RECV_BUFF_SZIE 10240
#endif 

// 为每个连接的客户端建立一个缓存区对象
class ClientSocket
{
public:
	ClientSocket(SOCKET sockfd = INVALID_SOCKET)
	{
		_sockfd = sockfd;
		memset(_szMsgBuf, 0, sizeof(_szMsgBuf));
		_lastPos = 0;
	}

	SOCKET sockfd()
	{
		return _sockfd;
	}

	char* msgBuf()
	{
		return _szMsgBuf;
	}

	int getLastPos()
	{
		return _lastPos;
	}
	void setLastPos(int pos)
	{
		_lastPos = pos;
	}

	//  发生数据
	int SendData(DataHeader* header)
	{
		if (header)
		{
			return send(_sockfd, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}

private:
	// socket fd_set  file desc set
	SOCKET _sockfd;
	//第二缓冲区 消息缓冲区
	char _szMsgBuf[RECV_BUFF_SZIE * 5];
	//消息缓冲区的数据尾部位置
	int _lastPos;
};

class INetEvent
{
public:
	//纯虚函数
	//客户端离开事件
	virtual void onNetJoin(ClientSocket *pClient) = 0;
	virtual void OnNetLeave(ClientSocket* pClient) = 0;
	virtual void OnNetMsg(ClientSocket * pClient, DataHeader* header) = 0;
private:

};


// 原本主线程中循环处理所有的客户端连接，如今将每个连接交给子线程，
// 为子线程封装一个对象，用于处理客户端连接
class CellServer
{
public:
	CellServer(SOCKET sock = INVALID_SOCKET)
	{
		_sock = sock;
		_pNetEvent = nullptr;  // 
		_clients_change = true;
	}

	~CellServer()
	{
		Close();
		_sock = INVALID_SOCKET;
	}
	// 这个是用于将虚基类指向不同实现的子类
	void setEventObj(INetEvent* event)
	{
		_pNetEvent = event;
	}

	//关闭Socket
	void Close()
	{
		if (_sock != INVALID_SOCKET)
		{
#ifdef _WIN32
			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{
				closesocket(_clients[n]->sockfd());
				delete _clients[n];
			}
			// 8 关闭套节字closesocket
			closesocket(_sock);  //清除Windows socket环境交给主线程去完成
#else
			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{
				close(_clients[n]->sockfd());
				delete _clients[n];
			}
			// 8 关闭套节字closesocket
			close(_sock);
#endif
			_clients.clear();
		}
	}

	//是否工作中
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}


	bool OnRun()
	{
		while (isRun())
		{
			// 有新客户端加入
			// 再把客户端从_clientsBuff转移到_clients,从缓冲队列里取出客户数据
			if (_clientsBuff.size() > 0)
			{
				_clients_change = true;
				// 自解锁
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pClient : _clientsBuff)
				{
					_clients[pClient->sockfd()] = pClient;
				}
				_clientsBuff.clear();
			}

			//如果没有需要处理的客户端，就跳过
			if (_clients.empty())
			{
				// 线程休眠1ms，防止不断进入这个判断，导致资源的浪费。
				// 采用c++11的定时对象，创建一个1ms的对象
				std::chrono::milliseconds t(1);
				// 采用c++11的线程函数，对当前线程休眠1ms
				std::this_thread::sleep_for(t);
				continue;
			}

			//伯克利套接字 BSD socket
			fd_set fdRead;//描述符（socket） 集合
						  //清理集合
			FD_ZERO(&fdRead);
			// 只有当客户端集合发送改变时，再像监听事件集合中放东西
			if (_clients_change) {
				_clients_change = false;
				_maxSock = _clients.begin()->second->sockfd();
				for (auto iter : _clients)
				{
					FD_SET(iter.second->sockfd(), &fdRead);
					if (_maxSock < iter.second->sockfd())
					{
						_maxSock = iter.second->sockfd();
					}
				}
				memcpy(&_fdRead_bak, &fdRead, sizeof(fd_set));
			}
			else {
				memcpy(&fdRead, &_fdRead_bak, sizeof(fd_set));
			}

			///nfds 是一个整数值 是指fd_set集合中所有描述符(socket)的范围，而不是数量
			///既是所有文件描述符最大值+1 在Windows中这个参数可以写0
			int ret = select(_maxSock + 1, &fdRead, nullptr, nullptr, nullptr);
			if (0 > ret)
			{
				printf("select任务结束。\n");
				Close();
				return false;
			}
			else if (0 == ret) {
				continue;
			}

#ifdef _WIN32
			for (int n = 0; n < fdRead.fd_count; n++)
			{
				auto iter = _clients.find(fdRead.fd_array[n]);
				if (iter != _clients.end())
				{
					if (-1 == RecvData(iter->second))
					{
						if (_pNetEvent)
							_pNetEvent->OnNetLeave(iter->second);
						_clients_change = true;
						_clients.erase(iter->first);
					}
				}
				else {
					printf("error. if (iter != _clients.end())\n");
				}

			}
#else
			std::vector<ClientSocket*> temp;
			for (auto iter : _clients)
			{
				if (FD_ISSET(iter.second->sockfd(), &fdRead))
				{
					if (-1 == RecvData(iter.second))
					{
						if (_pNetEvent)
							_pNetEvent->OnNetLeave(iter.second);
						_clients_change = false;
						temp.push_back(iter.second);
					}
				}
			}
			for (auto pClient : temp)
			{
				_clients.erase(pClient->sockfd());
				delete pClient;
			}
#endif
		}
	}
	//缓冲区
	char _szRecv[RECV_BUFF_SZIE] = {};
	//接收数据 处理粘包 拆分包
	int RecvData(ClientSocket* pClient)
	{
		// 5 接收客户端数据
		int nLen = (int)recv(pClient->sockfd(), _szRecv, RECV_BUFF_SZIE, 0);
		//printf("nLen=%d\n", nLen);
		if (nLen <= 0)
		{
			printf("客户端<Socket=%d>已退出，任务结束。\n", pClient->sockfd());
			return -1;
		}
		//将收取到的数据拷贝到消息缓冲区
		memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);
		//消息缓冲区的数据尾部位置后移
		pClient->setLastPos(pClient->getLastPos() + nLen);

		//判断消息缓冲区的数据长度大于消息头DataHeader长度
		while (pClient->getLastPos() >= sizeof(DataHeader))
		{
			//这时就可以知道当前消息的长度
			DataHeader* header = (DataHeader*)pClient->msgBuf();
			//判断消息缓冲区的数据长度大于消息长度
			if (pClient->getLastPos() >= header->dataLength)
			{
				//消息缓冲区剩余未处理数据的长度
				int nSize = pClient->getLastPos() - header->dataLength;
				//处理网络消息
				OnNetMsg(pClient, header);
				//将消息缓冲区剩余未处理数据前移
				memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, nSize);
				//消息缓冲区的数据尾部位置前移
				pClient->setLastPos(nSize);
			}
			else {
				//消息缓冲区剩余数据不够一条完整消息
				break;
			}
		}
		return 0;
	}

	//响应网络消息
	virtual void OnNetMsg(ClientSocket *pClient, DataHeader* header)
	{
		_pNetEvent->OnNetMsg(pClient, header);
		//switch (header->cmd)
		//{
		//case CMD_LOGIN:
		//{

		//	Login* login = (Login*)header;
		//	//printf("收到客户端<Socket=%d>请求：CMD_LOGIN,数据长度：%d,userName=%s PassWord=%s\n", cSock, login->dataLength, login->userName, login->PassWord);
		//	//忽略判断用户密码是否正确的过程
		//	LoginResult ret;
		//	pClient->SendData(&ret);
		//}
		//break;
		//case CMD_LOGOUT:
		//{
		//	Logout* logout = (Logout*)header;
		//	//printf("收到客户端<Socket=%d>请求：CMD_LOGOUT,数据长度：%d,userName=%s \n", cSock, logout->dataLength, logout->userName);
		//	//忽略判断用户密码是否正确的过程
		//	LogoutResult ret;
		//	pClient->SendData(&ret);
		//}
		//break;
		//default:
		//{
		//	printf("<socket=%d>收到未定义消息,数据长度：%d\n", pClient->sockfd(), header->dataLength);
		//	DataHeader ret;
		//	pClient->SendData(&ret);
		//}
		//break;
		//}
	}

	void addClient(ClientSocket* pClient)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		//_mutex.lock();
		_clientsBuff.push_back(pClient);
		//_mutex.unlock();
	}
	

	// 创建并启动一个新的线程
	void Start()
	{
		// 线程的入口是类内方法，有一个隐藏的参数，this指针
		// mem_fn将成员函数转为函数对象，使用指针或者引用绑定
		_thread = std::thread(std::mem_fn(&CellServer::OnRun), this);
	}

	size_t getClientCount()
	{
		return _clients.size() + _clientsBuff.size();
	}



private:
	// 这个_sock只是为了标记主线程监听的套接字是哪个，在子线程目前没什么实际作用
	SOCKET _sock;
	//正式客户队列
	std::map<SOCKET, ClientSocket*> _clients;
	// std::vector<ClientSocket*> _clients;
	//缓冲客户队列
	std::vector<ClientSocket*> _clientsBuff;
	// 缓存队列锁
	std::mutex _mutex;
	// 在使用智能指针之前，尽量不要使用指针
	std::thread _thread;
	// 网络事件对象
	INetEvent* _pNetEvent;
private:
	// 备份读事件集合
	fd_set _fdRead_bak;
	// 标记读事件集合是否发生变化
	bool _clients_change;
	SOCKET _maxSock;
	
};

// 服务器主线程对象负责：建立连接和分发客户端，并且做一些统计性的数据处理
class EasyTcpServer : public INetEvent
{
private:
	SOCKET _sock;  // 服务器的监听套接字
	// 主线程对象不再记录所有连接的客户端
	// 服务器子线程对象
	std::vector<CellServer*> _cellServers;
	// 定时
	CELLTimestamp _tTime;
	// 计数，给到INetEvent去自增
	std::atomic_int _recvCount;
	// 客户端计数
	std::atomic_int _clientCount;
public:
	EasyTcpServer()
	{
		_sock = INVALID_SOCKET;
		_recvCount = 0;
		_clientCount = 0;
	}
	virtual ~EasyTcpServer()
	{
		Close();
	}
	//初始化Socket
	SOCKET InitSocket()
	{
#ifdef _WIN32
		//启动Windows socket 2.x环境
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		if (INVALID_SOCKET != _sock)
		{
			printf("<socket=%d>关闭旧连接...\n", (int)_sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			printf("错误，建立socket失败...\n");
		}
		else {
			printf("建立socket=<%d>成功...\n", (int)_sock);
		}
		return _sock;
	}

	//绑定IP和端口号
	int Bind(const char* ip, unsigned short port)
	{
		//if (INVALID_SOCKET == _sock)
		//{
		//	InitSocket();
		//}
		// 2 bind 绑定用于接受客户端连接的网络端口
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);//host to net unsigned short

#ifdef _WIN32
		if (ip) {
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		}
#else
		if (ip) {
			_sin.sin_addr.s_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.s_addr = INADDR_ANY;
		}
#endif
		int ret = bind(_sock, (sockaddr*)&_sin, sizeof(_sin));
		if (SOCKET_ERROR == ret)
		{
			printf("错误,绑定网络端口<%d>失败...\n", port);
		}
		else {
			printf("绑定网络端口<%d>成功...\n", port);
		}
		return ret;
	}

	//监听端口号
	int Listen(int n)
	{
		// 3 listen 监听网络端口
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret)
		{
			printf("socket=<%d>错误,监听网络端口失败...\n", _sock);
		}
		else {
			printf("socket=<%d>监听网络端口成功...\n", _sock);
		}
		return ret;
	}

	//接受客户端连接
	SOCKET Accept()
	{
		// 4 accept 等待接受客户端连接
		sockaddr_in clientAddr = {};
		int nAddrLen = sizeof(sockaddr_in);
		SOCKET cSock = INVALID_SOCKET;
#ifdef _WIN32
		cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
		cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t *)&nAddrLen);
#endif
		if (INVALID_SOCKET == cSock)
		{
			printf("socket=<%d>错误,接受到无效客户端SOCKET...\n", (int)_sock);
		}
		else
		{
			//NewUserJoin userJoin;
			//SendDataToAll(&userJoin);
			// 将新加入的客户端，分给线程去处理。
			addClientToCellServer(new ClientSocket(cSock));
			
			//printf("socket=<%d>新客户端<%d>加入：socket = %d,IP = %s \n", (int)_sock, _clients.size(),(int)cSock, inet_ntoa(clientAddr.sin_addr));
		}
		return cSock;
	}


	// 将新加入的客户端，交给目前最少的第一个线程去处理
	void addClientToCellServer(ClientSocket* pClient)
	{
		// 客户端全部交由子线程去处理，主线程不再保留
		//查找客户数量最少的CellServer消息处理对象
		auto pMinServer = _cellServers[0];  // 初始化为第一个子线程对象
		for (auto pCellServer : _cellServers)
		{
			if (pMinServer->getClientCount() > pCellServer->getClientCount())
			{
				pMinServer = pCellServer;
			}
		}
		pMinServer->addClient(pClient);
		onNetJoin(pClient);
	}

	void Start(unsigned int threadCount = 1)
	{
		// cell服务器对象作为线程入口
		// 服务器主线程备份服务器子线程对象
		for (int n = 0; n < threadCount; n++)
		{
			// 创建一个所谓的微服务器对象，通过_sock 标识该对象所属的监听套接字
			// cellServer是服务器子线程
			auto ser = new CellServer(_sock);
			// 放入主线程的服务器线程池
			_cellServers.push_back(ser);
			// 将虚基类指向了子类EasyTcpServer，从而调用EasyTcpServer重写的OnNetMsg方法
			// 称之为注册网络事件
			ser->setEventObj(this);
			// 启动线程
			ser->Start();
		}
	}
	//关闭Socket
	void Close()
	{
		if (_sock != INVALID_SOCKET)
		{
			// 客户端的关闭全部放到子线程去
#ifdef _WIN32
			
			// 8 关闭套节字closesocket
			closesocket(_sock);
			//------------
			//清除Windows socket环境
			WSACleanup();
#else
			// 8 关闭套节字closesocket
			close(_sock);
#endif
		}
	}
	//处理网络消息
	//int _nCount = 0;
	bool OnRun()
	{
		if (isRun())
		{
			time4msg();
			//伯克利套接字 BSD socket
			fd_set fdRead;//描述符（socket） 集合
						  //fd_set fdWrite;
						  //fd_set fdExp;
						  //清理集合
			FD_ZERO(&fdRead);  //为了加快速度，只处理读事件，其他时间暂时忽略
			//FD_ZERO(&fdWrite);
			//FD_ZERO(&fdExp);
			//将描述符（socket）加入集合
			FD_SET(_sock, &fdRead);
			//FD_SET(_sock, &fdWrite);
			//FD_SET(_sock, &fdExp);
			///nfds 是一个整数值 是指fd_set集合中所有描述符(socket)的范围，而不是数量
			///既是所有文件描述符最大值+1 在Windows中这个参数可以写0
			timeval t = { 0,10 };  // 最大阻塞时间10us
			// windows的第一个参数不需要计算所有套接字的最大值
			int ret = select(_sock + 1, &fdRead, 0, 0, &t); //
															//printf("select ret=%d count=%d\n", ret, _nCount++);
			if (ret < 0)
			{
				printf("select任务结束。\n");
				Close();
				return false;
			}
			//判断是否有新客户端接入，描述符（socket）是否在集合中
			if (FD_ISSET(_sock, &fdRead))
			{
				FD_CLR(_sock, &fdRead);
				// Accept()，成功返回时，完成第三次握手，同客户端建立连接
				Accept();
				return true;
			}
			// 主线程不再轮询是哪个客户端触发了事件，而是一旦有新客户端，就将新客户端分发给子线程处理。
			return true;
		}
		return false;
	}
	//是否工作中
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}

	//记录每秒处理的消息数量
	void time4msg()
	{
		auto t1 = _tTime.getElapsedTimeSecond();
		// 每过去一秒，输出一次接受的消息的信息
		if (t1 >= 1.0)
		{
			// 第几个线程，时间，服务器的监听套接字，客户端的数量，每秒所有线程总共处理的数据包数（每一个登录消息体，处理一次）。
			printf("thread<%d>,time<%lf>,socket<%d>,clients<%d>,recvCount<%d>\n", _cellServers.size(), t1, _sock, (int)_clientCount, (int)(_recvCount / t1));
			_recvCount = 0;
			_tTime.update();
		}
	}

	//发送指定Socket数据


	// 群发的可能几乎没有
	//void SendDataToAll(DataHeader* header)
	//{
	//	for (int n = (int)_clients.size() - 1; n >= 0; n--)
	//	{
	//		_clients[n]->SendData(header);
	//	}
	//}

	// 将 pclient 从某个线程监听的客户端集合中移除
	virtual void OnNetLeave(ClientSocket* pClient)
	{
		// 主线程不再清除连接的客户端
		_clientCount--;

	}
	virtual void OnNetMsg(ClientSocket * pClient, DataHeader* header)
	{
		_recvCount++;
	}
	virtual void onNetJoin(ClientSocket *pClient) {
		_clientCount++;
	}
};

#endif // !_EasyTcpServer_hpp_
