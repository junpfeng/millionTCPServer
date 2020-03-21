#define WIN32_LEAN_AND_MEAN  // 避免引入早期的重复定义
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include<windows.h>
#include<WinSock2.h>
#include <stdio.h>
#include <thread>  // c++11 加入的thread库
// #pragma comment(lib, "ws2_32.lib")

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
	CMD_NEW_USER_JOIN,  // 新用户加入
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


// 当有新客户端加入时，就需要群发给其他已经加入的客户端。
struct NewUserJoin :public DataHeader {
	NewUserJoin() {
		dataLength = sizeof(NewUserJoin);
		cmd = CMD_NEW_USER_JOIN;
		sock = 0;
	}
	int sock;
};

bool g_brun = true;

int processor(SOCKET _cSock);
void cmdThread(SOCKET _sock);

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

	// 启动线程
	std::thread t1(cmdThread, _sock);
	t1.detach();  // 将 t1 线程和当前线程分离，即不需要主线程回收。
	while (g_brun) {
		// 客户端添加 select 网络模型
		fd_set fdReads;
		FD_ZERO(&fdReads);
		FD_SET(_sock, &fdReads);
		timeval t = { 1, 0 };  // 阻塞时长上限1s
		int ret = select(_sock, &fdReads, NULL, NULL, &t);  
		if (ret < 0) {
			printf("select 出错\n");
			break;
		}
		if (FD_ISSET(_sock, &fdReads)) { // 客户端收到数据
			FD_CLR(_sock, &fdReads);
			if (-1 == processor(_sock)) {
				printf("服务器断开连接\n");
				break;
			}
		}

	}
	// 7. 关闭套接字
	closesocket(_sock);
	// --------------
	// Windows网络开发框架
	// 清除 socket 环境
	WSACleanup();
	printf("quit\n");
	getchar();
	return 0;
}

int processor(SOCKET _cSock)
/*
输入：客户端连接套接字
处理：xxx
返回：是否处理成功 -1/0
*/
{

	// 建立一个缓冲区
	char szRecv[1024] = { 0 };
	// 5.接受客户端数据
	int nlen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
	DataHeader * header = (DataHeader*)szRecv;  // 包头指针指向缓冲区
	if (nlen <= 0) {
		printf("server socket = %d quit\n", _cSock);
		return -1;
	}
	// 解析数据头
	//printf("收到命令：%d 数据长度 %d\n", header.cmd, header.dataLength);
	switch (header->cmd)
	{
	case CMD_LOGIN_RESULT: {
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		LoginResult * login = (LoginResult  *)szRecv;
		printf("收到命令:CMD_LOGIN_RESULT, 数据长度:%d\n", login->dataLength);
	}break;
	case CMD_LOGOUT_RESULT:
	{
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		LogoutResult * logout = (LogoutResult*)szRecv;
		printf("收到命令:CMD_LOGOUT_RESULT, 数据长度:%d\n", logout->dataLength);
	}break;
	case CMD_NEW_USER_JOIN: {
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		NewUserJoin * userjoin = (NewUserJoin*)szRecv;
		printf("有新客户端加入:socket = %d, 数据长度:%d\n", userjoin->sock, userjoin->dataLength);
	}break;
	default:
		header->cmd = CMD_ERROR;
		header->dataLength = 0;
		send(_cSock, (char*)&header, sizeof(header), 0);
		break;
	}
	return 0;
}

void cmdThread(SOCKET _sock) {
	while (1) {
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit")) {
			printf("退出\n");
			g_brun = false;
			return;
		}
		else if (0 == strcmp(cmdBuf, "login")) {
			Login login;
			strcpy(login.userName, "xinyueox");
			strcpy(login.PassWord, "123");
			send(_sock, (const char *)&login, sizeof(Login), 0);
		}
		else if (0 == strcmp(cmdBuf, "logout")) {
			Logout logout;
			strcpy(logout.userName, "xinyueox");
			send(_sock, (const char *)&logout, sizeof(Logout), 0);
		}
		else {
			printf("未知的命令\n");
		}
	}
}
