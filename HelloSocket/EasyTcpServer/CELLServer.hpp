#ifndef _CELL_SERVER_HPP_
#define _CELL_SERVER_HPP_

#include"CELL.hpp"
#include"INetEvent.hpp"
#include"CELLClient.hpp"
#include "CELLSemaphore.hpp" 

#include<vector>
#include<map>

//网络消息接收处理服务类
class CellServer
{
public:
	// CellServer(SOCKET sock = INVALID_SOCKET)
	CellServer(int id) :_id(id)
	{
		_pNetEvent = nullptr;
		_taskServer.serverId = id;
	}

	~CellServer()
	{
		Close();
		// _sock 由 EasyTcpServer 维护
	}

	void setEventObj(INetEvent* event)
	{
		_pNetEvent = event;
	}
	
	//关闭Socket
	void Close()
	{
		if (_isRun) {
			printf("CellServer start Close %d\n", _id);
			// 继续去关闭任务线程
			_taskServer.Close();
			// 控制线程函数结束循环
			_isRun = false;
			// 阻塞等待线程函数结束
			_sem.wait();
			printf("CellServer end Close %d\n", _id);
		}
		
	}
	////是否工作中
	//bool isRun()
	//{
	//	// 一旦 _sock == INVALID_SOCKET，该对象启动的线程就会退出。
	//	// return _sock != INVALID_SOCKET;
	//	return _isRun;
	//}


	void OnRun()
	{
		while (_isRun)
		{
			if (!_clientsBuff.empty())
			{//从缓冲队列里取出客户数据
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pClient : _clientsBuff)
				{
					_clients[pClient->sockfd()] = pClient;
					pClient->serverId = _id;
					if (_pNetEvent)
						_pNetEvent->OnNetJoin(pClient);
					
				}
				// std::vector<CellClient*>().swap(_clientsBuff);
				_clientsBuff.clear();  // 此处使用clear是为了避免之后再次给这个缓冲区添加元素时出现的开辟内存的消耗
				_clients_change = true;
			}

			//如果没有需要处理的客户端，就跳过
			if (_clients.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				//旧的时间戳
				_oldTime = CELLTime::getNowInMilliSec();
				continue;
			}

			//伯克利套接字 BSD socket
			fd_set fdRead;//描述符（socket） 集合
						  //清理集合
			FD_ZERO(&fdRead);
			if (_clients_change)
			{
				_clients_change = false;
				//将描述符（socket）加入集合
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
			timeval t{ 0,1 };
			int ret = select(_maxSock + 1, &fdRead, nullptr, nullptr, &t);
			if (ret < 0)
			{
				printf("select任务结束。\n");
				Close();
				return;
			}
			//else if (ret == 0)
			//{
			//	continue;
			//}
			ReadData(fdRead);
			CheckTime(); // 检查所有与服务器相连的客户端的心跳计数是否超时
		}
		// 清空clients数组
		CleanClients();
		_sem.wakeup();
	}
	
	void CheckTime()
	{
		//当前时间戳
		auto nowTime = CELLTime::getNowInMilliSec();
		auto dt = nowTime - _oldTime;
		_oldTime = nowTime;

		for (auto iter = _clients.begin(); iter != _clients.end(); )
		{
			// 检测所有客户端的心跳计数，如果超时就移除
			if (iter->second->checkHeart(dt))
			{
				if (_pNetEvent)  // 客户端离开网络事件
					_pNetEvent->OnNetLeave(iter->second);
				_clients_change = true;
				printf("断开与 socket=%d\n", iter->second->sockfd());
				delete iter->second;
				// CloseSocket(iter->first);  // 断开与客户端的连接
				auto iterOld = iter;
				++iter;
				_clients.erase(iterOld);
				continue;
			}
			//  定时发送
			iter->second->checkSend(dt);
			++iter;
		}
	}

	void ReadData(fd_set& fdRead)
	{
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
						printf("断开与 socket=%d\n", iter->second->sockfd());
						delete iter->second;
						// closesocket(iter->first);
						_clients.erase(iter);
					}
				}
				else {
					printf("error. if (iter != _clients.end())\n");
				}
			}
#else
			std::vector<CellClient*> temp;
			for (auto iter : _clients)
			{
				if (FD_ISSET(iter.second->sockfd(), &fdRead))
				{
					if (-1 == RecvData(iter.second))
					{
						if (_pNetEvent)
							_pNetEvent->OnNetLeave(iter.second);
						_clients_change = true;
						// close(iter->first);
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

	//接收数据 处理粘包 拆分包
	int RecvData(CellClient* pClient)
	{
		//接收客户端数据
		char* szRecv = pClient->msgBuf() + pClient->getLastPos();
		int nLen = (int)recv(pClient->sockfd(), szRecv, (RECV_BUFF_SZIE)-pClient->getLastPos(), 0);
		_pNetEvent->OnNetRecv(pClient);
		//printf("nLen=%d\n", nLen);
		if (nLen <= 0)
		{
			//printf("客户端<Socket=%d>已退出，任务结束。\n", pClient->sockfd());
			return -1;
		}
		//将收取到的数据拷贝到消息缓冲区
		//memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);
		//消息缓冲区的数据尾部位置后移
		pClient->setLastPos(pClient->getLastPos() + nLen);

		//判断消息缓冲区的数据长度大于消息头netmsg_DataHeader长度
		while (pClient->getLastPos() >= sizeof(netmsg_DataHeader))
		{
			//这时就可以知道当前消息的长度
			netmsg_DataHeader* header = (netmsg_DataHeader*)pClient->msgBuf();
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
	virtual void OnNetMsg(CellClient* pClient, netmsg_DataHeader* header)
	{
		_pNetEvent->OnNetMsg(this, pClient, header);
	}

	void addClient(CellClient* pClient)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		//_mutex.lock();
		_clientsBuff.push_back(pClient);
		//_mutex.unlock();
	}

	void Start()
	{
		if (!_isRun) {
			_isRun = true;
			// 启动CellServer对象内的线程函数后，再次启动_taskServer对象内的线程函数
			_thread = std::thread(std::mem_fn(&CellServer::OnRun), this);
			_thread.detach();
			_taskServer.Start();
		}
	}

	size_t getClientCount()
	{
		return _clients.size() + _clientsBuff.size();
	}

	//void addSendTask(CellClient* pClient, netmsg_DataHeader* header)
	//{
	//	_taskServer.addTask([pClient, header]() {
	//		pClient->SendData(header);
	//		delete header;
	//	});
	//}
private:
	void CleanClients() {
		// 清空_clients 数组
		for (auto iter : _clients)
		{
			printf("CleanClients 断开与 socket=%d\n", iter.second->sockfd());
			delete iter.second;
		}
		_clients.clear();  // map的clear会真正的删除内存
						   // std::map<SOCKET, CellClient*>().swap(_clients);

		for (auto &iter : _clientsBuff) {
			delete iter;
		}
		// vector的clear 不会真正删除内存
		std::vector<CellClient*>(_clientsBuff).swap(_clientsBuff);
		// _clientsBuff.clear();
		// 一旦 _sock 被置为INVALID，该对象开启的线程就会结束循环，从而退出。
	}

private:
	// 服务器监听套接字，由EasyTcpServer对象传入
	// 但是由于是值传递，实际上CellServer还是可以独立管理_sock
	// SOCKET _sock;
	//正式客户队列
	std::map<SOCKET, CellClient*> _clients;
	//缓冲客户队列
	std::vector<CellClient*> _clientsBuff;
	//缓冲队列的锁
	std::mutex _mutex;
	std::thread _thread;
	//网络事件对象
	INetEvent* _pNetEvent = nullptr;
	//
	CellTaskServer _taskServer;

	//处理网络消息
	//备份客户socket fd_set
	fd_set _fdRead_bak;
	//客户列表是否有变化
	bool _clients_change = true;
	SOCKET _maxSock;
	//旧的时间戳
	time_t _oldTime = CELLTime::getNowInMilliSec();
	// 当前对象ID
	int _id = 0;
	// 是否正在工作标志位
	bool _isRun = false;
	// 信号量
	CELLSemaphore _sem;

};

#endif // !_CELL_SERVER_HPP_
