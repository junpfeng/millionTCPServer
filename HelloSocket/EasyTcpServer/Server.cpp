#define WIN32_LEAN_AND_MEAN  // 避免引入早期的重复定义
// #pragma comment(lib, "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include<windows.h>
#include<WinSock2.h>


// 定义结构化数据结构体
// 注意点：1.客户端和服务器要保证系统位数相同，2.字节序相同
struct DataPackage {
	int age;
	char name[32];
};

// 定义正式的数据包
enum CMD {  // 定义数据类型
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_ERROR
};
struct DataHeader {  // 作为所有数据报文的基类
	CMD cmd;
	short dataLength;  // 数据长度一般不大于 65535
};
struct Login :public DataHeader {  // 登录
	Login() {
		dataLength = sizeof(Login);
		cmd = CMD_LOGIN;
	}
	char userName[32];
	char PassWord[32];
};
struct LoginResult :public DataHeader {  //登录结果
	LoginResult() {
		dataLength = sizeof(LoginResult);
		cmd = CMD_LOGIN_RESULT;
		result = 0;
	}
	int result;
};
struct Logout :public DataHeader { // 登出
	Logout() {
		dataLength = sizeof(Logout);
		cmd = CMD_LOGOUT;
	}
	char userName[32];
};
struct LogoutResult :public DataHeader {  //登出结果
	LogoutResult() {
		dataLength = sizeof(LogoutResult);
		cmd = CMD_LOGOUT_RESULT;
		result = 0;
	}
	int result;
};

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
	char msgBUF[] = "hello, lam server.";

	_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
	if (INVALID_SOCKET == _cSock) {
		printf("invalid client socket\n");
	}
	printf("新客户端IP: = %s \n", inet_ntoa(clientAddr.sin_addr));

	while (true) {
		DataHeader header;
		// 5.接受客户端数据
		int nlen = recv(_cSock, (char*)&header, sizeof(header), 0);
		if (nlen <= 0) {
			printf("client quit\n");
			break;
		}
		// 解析数据头
		//printf("收到命令：%d 数据长度 %d\n", header.cmd, header.dataLength);
		switch (header.cmd)
		{
			case CMD_LOGIN: {
				Login login;
				// 报文头前面已经收到了，这里要进行地址偏移。下同。
				recv(_cSock, (char*)&login + sizeof(header), sizeof(Login) - sizeof(header), 0);
				printf("收到命令:CMD_LOGIN, 数据长度:%d, usrname = %s, passwd = %s\n", login.dataLength, login.userName, login.PassWord);
				// 判断用户密码
				LoginResult ret; 
				send(_cSock, (char*)&ret, sizeof(LoginResult), 0);
			}
			break;
			case CMD_LOGOUT: 
			{
				Logout logout;
				recv(_cSock, (char*)&logout + sizeof(header), sizeof(Logout) - sizeof(header), 0);
				LogoutResult ret;
				send(_cSock, (char*)&ret, sizeof(ret), 0);
			}break;
			default:
				header.cmd = CMD_ERROR;
				header.dataLength = 0;
				send(_cSock, (char*)&header, sizeof(header), 0);
				break;
		}
	}

	// 8. 关闭套接字
	closesocket(_sock);

	// -------------------------
	// Windows网络开发框架
	WSACleanup();
	printf("quit\n");
	getchar();
	return 0;
}