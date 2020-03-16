#define WIN32_LEAN_AND_MEAN  // 避免引入早期的重复定义
// #pragma comment(lib, "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include<windows.h>
#include<WinSock2.h>




int main() {
	// Windows 网络开发框架
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);

	// -通用框架部分-
	// 1. 创建socket 套接字
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	// 2. 绑定网络端口
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);  // host to net unsigned short
	_sin.sin_addr.S_un.S_addr = INADDR_ANY; // 本机的任意地址都可以访问，inet_addr("127.0.0.1");
	if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(_sin))) {
		printf("bind error");
	}
	else {
		printf("bind successfully\n");
	}
	// 3. 监听网络端口
	if (SOCKET_ERROR == listen(_sock, 5)) {
		printf("listen error");
	}
	else {
		printf("listen successfully\n");
	}
	
	// 4. 阻塞等待客户端请求
	sockaddr_in clientAddr = {};
	int nAddrLen = sizeof(sockaddr_in);
	SOCKET _cSock = INVALID_SOCKET;
	//accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
	char msgBUF[] = "hello, lam server.";
	while (true) {
		_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
		if (INVALID_SOCKET == _cSock) {
			printf("invalid client socket\n");
		}
		printf("新客户端IP: = %s \n", inet_ntoa(clientAddr.sin_addr));
		// 5. 给客户端发送数据
		send(_cSock, msgBUF, strlen(msgBUF) + 1, 0);
	}

	// 6. 关闭套接字
	closesocket(_sock);

	// -------------------------
	// Windows网络开发框架
	WSACleanup();
	return 0;
}