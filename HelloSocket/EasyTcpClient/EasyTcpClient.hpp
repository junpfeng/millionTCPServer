#ifndef _EASYTCPCLIENT_HPP_
#define _EASYTCPCLIENT_HPP_
#ifdef _WIN32
	// #pragma comment(lib, "ws2_32.lib")
	#define FD_SETSIZE 1024
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

#include <stdio.h>
#include <vector>
#include <cstring>
#include "MessageHeader.hpp"

std::vector<SOCKET> g_clients;
// 缓冲区数据挖掘机的大小，为什么不直接用第二缓冲区接受内核缓冲区的数据？
// 答：因为第二缓冲区的数据可能还没处理完，直接接受会有覆盖的风险，因此采用挖掘机当中介
const unsigned int UNIT_BUFF_SZIE = 10240; // 10 kb 
// 第二缓冲区的大小
const unsigned int RECV_BUFF_SZIE = 102400; // 100 kb

class EasyTcpClient
{
	SOCKET _sock;
	char _unRecv[UNIT_BUFF_SZIE];
	char _RecvBuf[RECV_BUFF_SZIE];
	int _lastPos;  // 指定第二缓冲区的已经使用长度
public:
	EasyTcpClient() :_sock(INVALID_SOCKET), _unRecv{  }, _RecvBuf{  }, _lastPos(0) 
	{
		// 初始化socket

	}

	// 虚析构函数
	virtual ~EasyTcpClient() 
	{
		Close();
	}

	// 初始化socket
	void initSocket()
	{
#ifdef _WIN32
		// Windows 网络开发框架
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		// 1. 建立socket
		if (INVALID_SOCKET != _sock) {  // 如果已经指向一个打开的文件描述符，则关闭之
			printf("close old socket: %d\n", _sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (INVALID_SOCKET == _sock) {
			printf("build socket error\n");
		}
		else {
			printf("build socket successfully\n");
		}
		return;
	}

	// 连接服务器
	int Connect(const char *ip, unsigned short port)
	{
		// 2. 发起连接请求
		if (INVALID_SOCKET == _sock) {
			initSocket();
		}
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
#ifdef _WIN32
	    	_sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else	
	    	_sin.sin_addr.s_addr = inet_addr(ip);
#endif
		int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret) {
			printf("connect error\n");
		}
		else {
			printf("connect sucessfully\n");
		}
		return ret;
	}

	// 关闭 socket
	void Close()
	{
		if (_sock == INVALID_SOCKET)
			return;
#ifdef _WIN32
		// 7. 关闭套接字
		closesocket(_sock);
		// --------------
		// Windows网络开发框架
		// 清除 socket 环境
		WSACleanup();
#else
		close(_sock);
#endif
		_sock = INVALID_SOCKET;
	}

	// 发送数据
	int SendData(DataHeader * header) {
		if (isRun() && header) {  //_sock有效且header有效
			return send(_sock, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}

	// 接收数据，处理粘包、拆包
	int RecvData(SOCKET _cSock) {
		// 5.接受客户端数据
		// 先接受数据头
		int nlen = recv(_cSock, _unRecv, UNIT_BUFF_SZIE, 0);
		if (nlen <= 0) {
			printf("server socket = %d quit\n", _cSock);
			return -1;
		}
		// 将数据放到第二缓冲区
		memcpy(_RecvBuf + _lastPos, _unRecv, nlen);
		_lastPos += nlen; 
		// 判断第二缓冲区是否有完整的包头数据
		while (_lastPos >= sizeof(DataHeader)) {
			DataHeader * header = (DataHeader*)_RecvBuf;  // 包头指针指向缓冲区
			if (_lastPos >= header->dataLength) {
				// 第二缓冲区，剩余消息，提前保存
				int RemainSize = _lastPos - header->dataLength;
				OnNetMsg(header);
				memcpy(_RecvBuf, _unRecv + header->dataLength, RemainSize);
				_lastPos = RemainSize;
			}
			else {
				// the remain msg not enough
				break;
			}
		}
		return 0;
	}

	// 响应网络消息
	void OnNetMsg(DataHeader *header) {
		// 解析数据头
		//printf("收到命令：%d 数据长度 %d\n", header.cmd, header.dataLength);
		switch (header->cmd)
		{
			case CMD_LOGIN_RESULT: {
				LoginResult * login = (LoginResult  *)header;
				printf("收到命令:CMD_LOGIN_RESULT, 数据长度:%d\n", header->dataLength);
			}break;
			case CMD_LOGOUT_RESULT:
			{
				LogoutResult * logout = (LogoutResult*)header;
				printf("收到命令:CMD_LOGOUT_RESULT, 数据长度:%d\n", header->dataLength);
			}break;
			case CMD_NEW_USER_JOIN: {
				NewUserJoin * userjoin = (NewUserJoin*)header;
				printf("有新客户端加入:socket = %d, 数据长度:%d\n", userjoin->sock, header->dataLength);
			}break;
			case CMD_ERROR: {
				printf("<socket=%d>recv:CMD_ERROR, data length:%d\n", _sock, header->dataLength);
			}break;
			default:
				//header->cmd = CMD_ERROR;
				//header->dataLength = 0;
				//send(_sock, (char*)&header, sizeof(header), 0);
				printf("未定义消息\n");
				break;
			}
	}

	// 处理数据
	bool onRun() {

		// 判断 socket的有效性
		if (!isRun())
			return false;

		// 客户端添加 select 网络模型
		fd_set fdReads;
		FD_ZERO(&fdReads);
		FD_SET(_sock, &fdReads);
		timeval t = { 1, 0 };  // 阻塞时长上限1s
		int ret = select(_sock + 1, &fdReads, NULL, NULL, &t);
		if (ret < 0) {
			printf("<socket=%d> select 出错\n", _sock);
			return false;
		}
		if (FD_ISSET(_sock, &fdReads)) { // 客户端收到数据
			FD_CLR(_sock, &fdReads);
			// 监听到事件触发，就接受数据
			if (-1 == RecvData(_sock)) {
				printf("<socket=%d> 服务器断开连接\n", _sock);
				return false;
			}
		}
		return true;
	}
	//
	bool isRun() {
		return _sock == INVALID_SOCKET ? false : true;
	}
};


#endif
