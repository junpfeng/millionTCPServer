#ifndef __EASYTCPSERVER_HPP_
#define __EASYTCPSERVER_HPP_

#ifdef _WIN32
	// #pragma comment(lib, "ws2_32.lib")
	#define FD_SETSIZE 10240  // 设置这个宏，修改中select最大复用套接字
	#define WIN32_LEAN_AND_MEAN  // 避免引入早期的重复定义
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
	#include<windows.h>
	#include<WinSock2.h>
#else
	#include <unistd.h>
	#include <arpa/inet.h>

	#define SOCKET int
	#define INVALID_SOCKET (SOCKET)(~0)
	#define SOCKET_ERROR   (-1)
#endif

#include <vector>
#include <stdio.h>
#include "MessageHeader.hpp"
#include "CELLTimestamp.hpp"

// 缓冲区数据挖掘机的大小，为什么不直接用第二缓冲区接受内核缓冲区的数据？
// 答：因为第二缓冲区的数据可能还没处理完，直接接受会有覆盖的风险，因此采用挖掘机当中介
const unsigned int RECV_BUFF_SZIE = 10240; // 10 kb 
// 第二缓冲区的大小
const unsigned int MSG_BUFF_SZIE = 102400; // 100 kb

class clientSocket {
private:
	SOCKET _cSock;  // 每个客户端数据类，使用 _cSock 标记哪个客户端
	char _MsgBuf[MSG_BUFF_SZIE];
	unsigned long long _lastPos;  // 指定第二缓冲区的已经使用长度

public:
	clientSocket(SOCKET cSock = INVALID_SOCKET) :_cSock(cSock), 
		_lastPos(0), _MsgBuf{} 
	{}
	SOCKET getcSock() {
		return _cSock;
	}
	char * getMsgBuf() {
		return _MsgBuf;
	}

	int getLastPos() {
		return _lastPos;
	}

	void setLastPos(int pos) {
		_lastPos = pos;
	}
};

class EasyTcpServer
{
	/*
	* 1.初始化套接字（即建立监听套接字）
	* 2.绑定端口
	* 3.监听端口
	* 4.IO复用处理连接和数据
	* 5.关闭所有打开的socket
	* 问题1：为什么第三步3.监听端口不使用IO复用技术呢？
	*		答：因为初次监听端口时，没有其他事情需要做，即使阻塞住也没关系啊。
	*/
private:
	SOCKET _sock;
	std::vector<clientSocket *> _clients;
	char _RecvBuf[RECV_BUFF_SZIE];
	CELLTimestamp _tTime;
	unsigned int _recvCount;
public:
	EasyTcpServer() :_sock(INVALID_SOCKET), _RecvBuf{}
	{

	}
	virtual ~EasyTcpServer() {

	}

	// 初始化
	void initSocket() {
#ifdef _WIN32
		// Windows 网络开发框架
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		// -通用框架部分-
		// 1. 创建socket 套接字
		if (ValidSocket(_sock)) {
			printf("<socket=%d>关闭旧连接\n", _sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock) {
			printf("建立socket失败\n");
		}else {
			printf("建立<socket=%d>成功\n");
		}
	}

	// 绑定端口
	int Bind(const char * ip, unsigned short port) {
		// 异常处理第一步
		unsigned long IP = INADDR_ANY;
		if (nullptr != ip) {
			IP = inet_addr(ip);
		}	
		// 2. 绑定网络端口
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);  // host to net unsigned short
#ifdef _WIN32
		_sin.sin_addr.S_un.S_addr = IP; // 本机的任意地址都可以访问，inet_addr("127.0.0.1");
#else
		_sin.sin_addr.s_addr = IP;
#endif 
		if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(_sin))) {
			printf("bind <port=%d> error", port);
			return SOCKET_ERROR;
		}
		else {
			printf("bind <port=%d>successfully\n", port);
			return 0;
		}
	}
	// 监听端口
	int Listen(unsigned int n) {  // 支持最多n长的监听队列
		// 3. 监听网络端口
		if (SOCKET_ERROR == listen(_sock, n)) {
			printf("listen<socket=%d> error\n", _sock);
			return SOCKET_ERROR;
		}
		else {
			printf("listen <socket = %d> successfully\n", _sock);
			return 0;
		}
	}

	// 接受客户端的连接请求
	SOCKET Accept() {
		sockaddr_in clientAddr = {};
		int nAddrLen = sizeof(clientAddr);
		SOCKET _cSock = INVALID_SOCKET;
		_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);  // 同客户端建立连接
		if (!ValidSocket(_cSock)) {
			printf("invalid socket\n");
			return INVALID_SOCKET;
		}else {  
			NewUserJoin userjoin;
			//SendDataToAll(userjoin);  // 将新客户端加入的消息，群发出去
			_clients.push_back(new clientSocket(_cSock));
			printf("new joiner<number %d> :socket = %d, IP = %s \n", _clients.size(), (int)_cSock, inet_ntoa(clientAddr.sin_addr));
		}
		return _cSock;
	}
	// 向所有客户端群发消息，参数采用的父类引用
	void SendDataToAll(DataHeader & header) {
		for (int n = (int)_clients.size() - 1; n >= 0; --n) {
			SendData(_clients[n]->getcSock(), &header);
		}
	}
	// 接听数据
	int WaitNetMsg() {
		// select模型
		fd_set fdRead;
		fd_set fdWrite;
		fd_set fdExp;
		// 初始化事件描述符集合
		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExp);
		// 添加事件
		FD_SET(_sock, &fdRead);  //将监听的socket加入读事件集合中
		FD_SET(_sock, &fdWrite);
		FD_SET(_sock, &fdExp);
		SOCKET maxSock = _sock;
		for (auto &x : _clients) {  //将需要监听的客户端socket套接字加入监听集合，第一次进入是没有的
			FD_SET(x->getcSock(), &fdRead);  // 所有客户端集合的套接字都是需要监听的
			if (maxSock < x->getcSock())
				maxSock = x->getcSock();
		}

		timeval t = {1,0};  // 非阻塞
		int ret = select(_sock + 1, &fdRead, &fdWrite, &fdExp, &t);  // 设置最大阻塞事件1s
		if (ret < 0) {
			printf("select 出错\n");
			return ret;
		}else if (FD_ISSET(_sock, &fdRead)) {  // 如果是_sock套接字触发了事件，说明有新的连接请求，调用 accept
			FD_CLR(_sock, &fdRead);  // 一旦监听到某个套接字触发了事件，就先将其从监听事件集合中清除，因为每次调用这个函数时，会将监听套接字添加进来的
			Accept();  // 接受新的连接请求
		}

		// 虽然新加入的客户端已经被加入服务端列表中，但是客户端是先建立连接的，再所有客户端建立完连接之前，没有发送其他消息。
		for (int n = (int)_clients.size() - 1; n >= 0; --n) {
			// 轮询判断是不是当前socket造成的事件触发
			if (FD_ISSET(_clients[n]->getcSock(), &fdRead)) {
				// 异常判断，客户端断开
				if (-1 == RecvData(_clients[n]))
				{
					auto iter = _clients.begin() + n;
					if (iter != _clients.end()) {
						delete _clients[n];
						_clients.erase(iter);
						
					}
				}
			}
		}
		// printf("处理其他主线程业务...\n");
		return 0;
	}

	// 处理数据体
	virtual void ProcessNetMsg(SOCKET _cSock, DataHeader *header) {
		// 每次调用该函数，表示读入了一次数据
		_recvCount++;
		auto t1 = _tTime.getElapsedTimeSecond();
		// 每隔一秒，记录一次
		if (t1 >= 1.0) {
			printf("socket<%lf>, socket<%d>,_recvCount<%d>, connectedNum<%d>\n", t1, _sock, _recvCount, _clients.size());
			_recvCount = 0;
			_tTime.update();
		}

		// 解析数据头
		//printf("收到命令：%d 数据长度 %d\n", header.cmd, header.dataLength);
		switch (header->cmd)
		{
		case CMD_LOGIN: {	
			//Login * login = (Login*)header;
			//printf("收到命令:CMD_LOGIN, 数据长度:%d, usrname = %s, passwd = %s\n", login->dataLength, login->userName, login->PassWord);
			// 判断用户密码，实际上还没做处理
			//DataHeader * header = new LoginResult;
			//SendData(_cSock, header);
		}break;
		case CMD_LOGOUT:
		{
			//Logout * logout = (Logout*)header;
			//printf("收到命令:CMD_LOGOUT, 数据长度:%d, usrname = %s\n", logout->dataLength, logout->userName);
			//DataHeader * header = new LogoutResult;
			//SendData(_cSock, header);
		}break;
		default:
			printf("<socket = %d>收到未定义数据\n", _cSock);
			break;
		}
	}
	// 发送数据
	int SendData(SOCKET _cSock, DataHeader * header) {
		if (INVALID_SOCKET == _cSock || header == nullptr)
		{
			return SOCKET_ERROR;
		}
		return send(_cSock, (const char*) header, header->dataLength, 0);
	}


	int RecvData(clientSocket * cSock) {
		// 5.接受客户端数据
		// 将数据从内核缓冲区取出
		int nlen = recv(cSock->getcSock(), _RecvBuf, RECV_BUFF_SZIE, 0);
		if (nlen <= 0) {
			printf("server socket = %d quit\n", cSock->getcSock());
			return -1;
		}
		// 将数据放到第二缓冲区
		memcpy(cSock->getMsgBuf() + cSock->getLastPos(), _RecvBuf, nlen);
		cSock->setLastPos(cSock->getLastPos() + nlen);
		// 判断第二缓冲区是否有完整的包头数据
		while (cSock->getLastPos() >= sizeof(DataHeader)) {
			DataHeader * header = (DataHeader*)cSock->getMsgBuf();  // 包头指针指向缓冲区
			if (cSock->getLastPos() >= header->dataLength) {
				// 第二缓冲区，剩余消息，提前保存
				int RemainSize = cSock->getLastPos() - header->dataLength;
				while (RemainSize < 0) {
					int a = 1;
				}
				ProcessNetMsg(cSock->getcSock(), header);
				memcpy(cSock->getMsgBuf(), _RecvBuf + header->dataLength, RemainSize);
				cSock->setLastPos(RemainSize);
			}
			else {
				// the  remain msg not enough
				break;
			}
		}
	}

	// 关闭
	void Close() {
#ifdef _WIN32
		// -------------------------
		// Windows网络开发框架
		WSACleanup();
		// 8. 关闭全局客户端套接字集合
		for (int n = (int)_clients.size() - 1; n >= 0; --n) {
			if (ValidSocket(_clients[n]->getcSock())) {
				closesocket(_clients[n]->getcSock()); // 因为socket只是个数字，所以即使只是得到的返回值，也可以关闭
				delete _clients[n];
			}
		}
		if (ValidSocket(_sock))
			closesocket(_sock);  // 关闭监听套接字
#else
		// 8. 关闭套接字
		for (int n = (int)_clients.size() - 1; n >= 0; --n) {
			if (ValidSocket(_clients[n]->getcSock())) {
				close(_clients[n]->getcSock());
				delete _clients[n];
			}
		}
		if (ValidSocket(_sock))
			close(_sock);
#endif
		_clients.clear();
	}
	// 发送数据
	// 当前IP + 端口是否正在工作中
	bool ValidSocket(SOCKET _sock) {
		return INVALID_SOCKET == _sock ? false : true;
	}
};

#endif
 