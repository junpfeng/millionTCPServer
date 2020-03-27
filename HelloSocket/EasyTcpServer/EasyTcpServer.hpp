#ifndef __EASYTCPSERVER_HPP_
#define __EASYTCPSERVER_HPP_

#ifdef _WIN32
	// #pragma comment(lib, "ws2_32.lib")
	#define WIN32_LEAN_AND_MEAN  // 避免引入早期的重复定义
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
	#include<windows.h>
	#include<WinSock2.h>
#else
	#include <unistd.h>
	#include <sys/select.h>
	#include <arpa/inet.h>

	#define SOCKET int
	#define INVALID_SOCKET (SOCKET)(~0)
	#define SOCKET_ERROR   (-1)
#endif

#include <vector>
#include <stdio.h>
#include "MessageHeader.hpp"
#include <algorithm>
#include <malloc.h>
std::vector<SOCKET> g_clients;


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
	
public:
	EasyTcpServer():_sock(INVALID_SOCKET)
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
			printf("<socket=%d>close old connection\n", _sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock) {
			printf("build socket failed\n");
		}else {
			printf("build <socket=%d> successfully\n",_sock);
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
		socklen_t nAddrLen = sizeof(clientAddr);
		SOCKET _cSock = INVALID_SOCKET;
		_cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);  // 同客户端建立连接
		if (!ValidSocket(_cSock)) {
			printf("invalid socket\n");
			return INVALID_SOCKET;
		}else {  // 成功后，将连接好的 客户端套接字加入 客户端集合
			for (auto & x : g_clients) {  // 向其他客户端群发添加新用户的消息。
				NewUserJoin userjoin;
				userjoin.sock = _cSock;
				send(x, (const char*)&userjoin, sizeof(NewUserJoin), 0);
			}
			g_clients.push_back(_cSock);
			printf("new joiner:socket = %d, IP = %s \n", (int)_cSock, inet_ntoa(clientAddr.sin_addr));
		}
		return _cSock;
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
		int maxfd = _sock;
		for (auto &x : g_clients) {  //将需要监听的客户端socket套接字加入监听集合，第一次进入是没有的
			FD_SET(x, &fdRead);  // 所有客户端集合的套接字都是需要监听的
			maxfd = std::max(maxfd, x);
		}

		// timeval t = {0,0};  // 非阻塞

		int ret = select(maxfd + 1, &fdRead, &fdWrite, &fdExp, NULL);  // socket阻塞监听
		if (ret < 0) {
			printf("select error\n");
			return ret;
		}else if (FD_ISSET(_sock, &fdRead)) {  // 如果是_sock套接字触发了事件，说明有新的连接请求，调用 accept
			FD_CLR(_sock, &fdRead);  // 一旦监听到某个套接字触发了事件，就先将其从监听事件集合中清除，因为每次调用这个函数时，会将监听套接字添加进来的
			Accept();  // 接受新的连接请求
		}
		// 轮询事件集合，寻找触发事件文件描述符
		for (auto &x : g_clients) { 
		    if (!FD_ISSET(x, &fdRead))  // 该文件描述符没有触发读事件集合
			continue;
		 
		    DataHeader * header = RecvData(x);
		    if (nullptr == header) {  // 返回nullptr，表示客户端已经断开连接
			auto iter = find(g_clients.begin(), g_clients.end(), x);  // 将断开连接的客户端套接字从客户端集合中剔除，
			if (iter != g_clients.end())
			g_clients.erase(iter);
		    }
		    else { // 否则处理事件
			ProcessNetMsg(x, header);
			free(header);
		    }
		}
		printf("processing another tasks...");
		return 0;
	}
	
	// 处理数据体
	virtual void ProcessNetMsg(SOCKET _cSock, DataHeader *header) {
		// 解析数据头
		//printf("收到命令：%d 数据长度 %d\n", header.cmd, header.dataLength);
		switch (header->cmd)
		{
		case CMD_LOGIN: {	
			Login * login = (Login*)header;
			printf("get cmd :CMD_LOGIN, data length:%d, usrname = %s, passwd = %s\n", login->dataLength, login->userName, login->PassWord);
			// 判断用户密码，实际上还没做处理
			DataHeader * header = new LoginResult;
			SendData(_cSock, header);
		}break;
		case CMD_LOGOUT:
		{
			Logout * logout = (Logout*)header;
			printf("get cmd:CMD_LOGOUT, data length:%d, usrname = %s\n", logout->dataLength, logout->userName);
			DataHeader * header = new LogoutResult;
			SendData(_cSock, header);
		}break;
		default:
			header->cmd = CMD_ERROR;
			header->dataLength = 0;
			SendData(_cSock, header);
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

	// 接受数据头
	DataHeader* RecvData(SOCKET _cSock) {
		// 建立一个缓冲区
		// char szRecv[1024] = { 0 };
		char * szRecv = (char*)malloc(sizeof(char) * 1024);
		// 5.接受客户端数据
		int nlen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
		DataHeader * header = (DataHeader*)szRecv;  // 包头指针指向缓冲区
		if (nlen <= 0) {
			printf("client socket = %d quit\n", _cSock);
			free(szRecv);
			return nullptr;
		}
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);  // 接收到的数据并未使用
		return header;
	}

	// 关闭
	void Close() {
#ifdef _WIN32
		// -------------------------
		// Windows网络开发框架
		WSACleanup();
		// 8. 关闭全局客户端套接字集合
		for (int n = (int)g_clients.size() - 1; n >= 0; --n) {
			if (ValidSocket(g_clients[n]))
				closesocket(g_clients[n]);
		}
		if (ValidSocket(_sock))
			closesocket(_sock);  // 关闭监听套接字
#else
		// 8. 关闭套接字
		for (int n = (int)g_clients.size() - 1; n >= 0; --n) {
			if (ValidSocket(g_clients[n]))
				close(g_clients[n]);
		}
		if (ValidSocket(_sock))
			close(_sock);
#endif
	}
	// 发送数据
	// 当前IP + 端口是否正在工作中
	bool ValidSocket(SOCKET _sock) {
		return INVALID_SOCKET == _sock ? false : true;
	}
};

#endif
 
