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

	// 当前对象结束时，相关资源关闭：线程和内存
	void Close()
	{
		CELLLog::Info("CellServer start Close %d\n", _id);
		// 继续去关闭任务线程
		_taskServer.Close();
		// 控制线程函数结束循环
		_thread.Close();
		CELLLog::Info("CellServer end Close %d\n", _id);


	}
	////是否工作中
	//bool isRun()
	//{
	//	// 一旦 _sock == INVALID_SOCKET，该对象启动的线程就会退出。
	//	// return _sock != INVALID_SOCKET;
	//	return _isRun;
	//}

	// 线程入口函数
	void OnRun(CELLThread * pThread)
	{
		while (pThread->isRun())
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

			// 如果没有需要处理的客户端，就跳过
			if (_clients.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				//旧的时间戳
				_oldTime = CELLTime::getNowInMilliSec();
				continue;
			}
			// 心跳检测和定时发送
			CheckTime(); // 检查所有与服务器相连的客户端的心跳计数是否超时

			//伯克利套接字 BSD socket
			fd_set fdRead;//描述符（socket） 集合
						  //清理集合
			fd_set fdWrite; // 可写事件集合

			// 如果客户端集合发生了改变，就更新读事件集合
			if (_clients_change)
			{
				FD_ZERO(&fdRead);
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

			// 需要被写的套接字和被读的不一定是同一个
			// memcpy(&fdWrite, &_fdRead_bak, sizeof(fd_set));
			bool bNeedWrite = false;
			FD_ZERO(&fdWrite);
			for (auto iter : _clients) {
				// 检测需要写的客户端
				if (iter.second->needWrite()) {
					// 一旦有一个客户端可写，即标志可写
					bNeedWrite = true;
					FD_SET(iter.second->sockfd(), &fdWrite);
				}
			}

			// nfds 是一个整数值 是指fd_set集合中所有描述符(socket)的范围，而不是数量
			// 既是所有文件描述符最大值+1 在Windows中这个参数可以写0
			// 最长等待 1 ms

			timeval t = { 0,1 };
			int ret = 0;

			// 如果发送缓冲区有数据，才会对发送事件进行监听
			// 但是客户端与多个客户端连接，无法通过判断单个客户端是否可写
			// 因此上面创建了一个标志位，一旦有一个客户端可写，即可写

			if (bNeedWrite)
			{
				ret = select(_maxSock + 1, &fdRead, &fdWrite, nullptr, &t);
			}
			else {
				ret = select(_maxSock + 1, &fdRead, nullptr, nullptr, &t);
			}

			if (ret < 0) // select 出错
			{
				CELLLog::Info("select任务结束。\n");
				pThread->Exit();
				// Close();
				break;
			}
			else {
				continue;
			}
			// 更新读事件和写事件集合
			ReadData(fdRead);
			WriteData(fdWrite);
			// CELLLog::Info("CELLServer%d.OnRun.select: fdRead=%d,fdWrite=%d\n", _id, fdRead.fd_count, fdWrite.fd_count);
		}
		// 清空clients数组由析构函数完成
		CELLLog::Info("CELLServer %d.OnRun exit\n", _id);
	}

	// 心跳检测的同时，进行定时发送
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
				CELLLog::Info("断开与 socket=%d\n", iter->second->sockfd());
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

	// 清空 pClient 数组
	void OnClientLeave(CellClient * pClient) {
		if (_pNetEvent) {
			_pNetEvent->OnNetLeave(pClient);
		}
		_clients_change = true;
		delete pClient;
	}

	// 更新写事件集合中的套接字
	void WriteData(fd_set & fdWrite) {
#ifdef _WIN32
		for (int n = 0; n < fdWrite.fd_count; ++n) {
			auto iter = _clients.find(fdWrite.fd_array[n]);
			if (iter != _clients.end()) {
				// 一旦发送出错，则退出
				if (-1 == iter->second->SendDataReal()) {
					OnClientLeave(iter->second);
					_clients.erase(iter);
				}
			}
		}
#else
		// 之所以不讲++iter放在for循环里，是因为循环体内会产生iter失效的清空
		for (auto iter = _clients.begin(); iter != _clients.end(); ) {
			// 
			if(iter->second->needWrite() && FD_ISSET(iter->second->sockfd(), &fdWrite)) {
				if (-1 == iter->second->SendDataReal()) {
					OnClientLeave(iter->second);
					auto iterOld = iter;
					++iter;
					// erase 操作会使得 iterOld 失效， 
					// 因此再erase之前，将iter++,
					// 由于map的iter之间无线性关系，因此可以防止其失效。
					_clients.erase(iterOld);
					continue;
	}
			}
			++iter;
		}

#endif
	}
	// 更新读事件集合中的套接字
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
						CELLLog::Info("断开与 socket=%d\n", iter->second->sockfd());
						delete iter->second;
						// closesocket(iter->first);
						_clients.erase(iter);
					}
				}
				else {
					CELLLog::Info("error. if (iter != _clients.end())\n");
				}
			}
#else
		for (auto iter = _clients.begin(); iter != _clients.end(); )
		{
			if (FD_ISSET(iter->second->sockfd(), &fdRead))
			{
				if (-1 == RecvData(iter->second))
				{
					OnClientLeave(iter->second);
					auto iterOld = iter;
					iter++;
					_clients.erase(iterOld);
					continue;
				}
			}
			iter++;
			}
#endif
	}

	//接收数据 处理粘包 拆分包
	// 接收的同时，立即响应和处理
	int RecvData(CellClient* pClient)
	{
		//接收客户端数据
		int nLen = pClient->RecvData();
		if (nLen <= 0) {
			return -1;
		}
		// 接收到网络消息计数+1
		_pNetEvent->OnNetRecv(pClient);
		// 将缓冲区内的数据全部处理完
		while (pClient->hasMsg()) {
			// 根据消息头，响应网络消息
			OnNetMsg(pClient, pClient->front_msg());
			// 弹出队首的数据（因为已经被处理过）
			pClient->pop_front_msg();
		}
		return 0;
	}

	//响应网络消息
	virtual void OnNetMsg(CellClient* pClient, netmsg_DataHeader* header)
	{
		_pNetEvent->OnNetMsg(this, pClient, header);
	}

	// 将新连接的客户端加入客户端缓冲区
	void addClient(CellClient* pClient)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		//_mutex.lock();
		_clientsBuff.push_back(pClient);
		//_mutex.unlock();
	}
	
	// 启动线程入口函数
	void Start()
	{
		_taskServer.Start();
		_thread.Start(
			// onCreate
			nullptr,
			// onRun
			[this](CELLThread * pThread) {
			OnRun(pThread);
		},
			//onDestory
			[this](CELLThread* pThread) {
			CleanClients();
		});
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
			CELLLog::Info("CleanClients 断开与 socket=%d\n", iter.second->sockfd());
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
	CELLThread _thread;
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
	// 信号量
	CELLSemaphore _sem;

};

#endif // !_CELL_SERVER_HPP_
