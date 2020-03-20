#define WIN32_LEAN_AND_MEAN  // 避免引入早期的重复定义
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include<windows.h>
#include<WinSock2.h>
#include <stdio.h>
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

	
	while (true) {

		// 3. 输入请求命令
		char cmdBUF[128] = {};
		scanf("%s", cmdBUF);
		// 4. 处理请求命令
		if (0 == strcmp(cmdBUF, "exit")) {
			printf("exit\n");
			break;
		}
		else if(0 == strcmp(cmdBUF, "login")){
			Login login;
			strcpy(login.userName, "junpfeng");
			strcpy(login.PassWord, "123");
			// 5. 向服务器发送请求登录
			send(_sock, (const char*)&login, sizeof(login), 0);

			LoginResult loginRet;
			// 接受服务器的登录结果
			recv(_sock, (char*)&loginRet, sizeof(loginRet), 0);
			printf("loginResult %d \n", loginRet.result);  
		}
		else if (0 == strcmp(cmdBUF, "logout")) {
			Logout logout;
			strcpy(logout.userName, "junpfeng");
			// 向接受服务器返回的数据
			send(_sock, (const char*)&logout, sizeof(logout), 0);
			// 接收服务器返回的数据
			LogoutResult logoutRet;
			recv(_sock, (char*)&logoutRet, sizeof(logoutRet), 0);
			printf("logoutResult :%d\n", logoutRet.result);
		}
		else {
			printf("命令不支持\n");
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