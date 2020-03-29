#ifndef _MESSAGEHEADER_HPP_
#define _MESSAGEHEADER_HPP_
#ifdef _WIN32
	#pragma comment(lib, "ws2_32.lib")
	#define WIN32_LEAN_AND_MEAN  // 避免引入早期的重复定义
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#include<windows.h>
	#include<WinSock2.h>
#else
	#include <unistd.h>
	#include <arpa/inet.h>

	#define SOCKET int
	#define INVALID_SOCKET (SOCKET)(~0)
	#define SOCKET_ERROR   (-1)
#endif

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
	DataHeader() {
		dataLength = sizeof(DataHeader);
		cmd = CMD_ERROR;
	}
	CMD cmd;
	unsigned long long dataLength;  // 单个TCP数据包的数据长度一般不大于 65535，但是连续的字节流就不止了。
};
struct Login :public DataHeader {  // 登录
	Login() {
		dataLength = sizeof(Login);
		cmd = CMD_LOGIN;
	}
	char userName[32];
	char PassWord[32];
	char dat[932];  // 数据填充，让 Login 占1k字节
};
struct LoginResult :public DataHeader {  //登录结果
	LoginResult() {
		dataLength = sizeof(LoginResult);
		cmd = CMD_LOGIN_RESULT;
		result = 0;
	}
	int result;
	char data[992];
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



#endif
