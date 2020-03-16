#define WIN32_LEAN_AND_MEAN  // 避免引入早期的重复定义
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<windows.h>
#include<WinSock2.h>
#include <stdio.h>
// #pragma comment(lib, "ws2_32.lib")

int main() {
	// Windows 网络开发框架
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);

	// -通用框架部分-
	// 1. 建立socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == _sock) {
		printf("build socket error\n");
	}

	// 2. 发起连接请求
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);
	_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
	if (SOCKET_ERROR == ret) {
		printf("connect error\n");
	}
	else {
		printf("connect sucessfully\n");
	}

	// 3.接受服务器信息 recv
	char recvBuf[256] = {};
	int nlen = recv(_sock, recvBuf, 256, 0);
	if (nlen > 0) {
		printf("接收到数据：%s\n", recvBuf);
	}
	else {
		printf("未曾接收到数据\n");
	}
	// 4. 关闭套接字
	closesocket(_sock);
	// --------------

	// Windows网络开发框架
	// 清除 socket 环境
	WSACleanup();
	getchar();
	return 0;
}