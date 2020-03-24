#ifdef _WIN32
    // #pragma comment(lib, "ws2_32.lib")
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
#include <stdio.h>
#include <vector>
#include <cstring>
#include <algorithm>
using std::max;
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

int processor(SOCKET _cSock);
std::vector<SOCKET> g_clients;

int main() {

#ifdef _WIN32
	// Windows 网络开发框架
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);
#endif
	// -通用框架部分-
	
	// 1. 创建socket 套接字
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	// 2. 绑定网络端口
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);  // host to net unsigned short

#ifdef _WIN32
	_sin.sin_addr.S_un.S_addr = INADDR_ANY; // 本机的任意地址都可以访问，inet_addr("127.0.0.1");
#else
	_sin.sin_addr.s_addr = INADDR_ANY;
#endif

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

	while (true) {

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

		SOCKET maxSock = _sock;  // 定义最大的描述符

		for (auto &x : g_clients) {  //将需要监听的客户端socket套接字加入监听集合，第一次进入是没有的
			FD_SET(x, &fdRead);
			maxSock = max(maxSock, x);  // 更新最大的文件描述符
		}

		// timeval t = {0,0};  // 非阻塞
		int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, NULL);  // socket阻塞监听
		if (ret < 0) {
			printf("select 出错\n");
			break;
		}
		if (FD_ISSET(_sock, &fdRead)) {  // 如果是_sock套接字触发了事件，说明有新的连接请求，调用 accept
			FD_CLR(_sock, &fdRead);
			sockaddr_in clientAddr = {};
			int nAddrLen = sizeof(clientAddr);
			SOCKET _cSock = INVALID_SOCKET;
			_cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);  // 同客户端建立连接
			if (INVALID_SOCKET == _cSock) {
				printf("invalid socket\n");
			}
			else {  // 成功后，将连接好的 客户端套接字加入 客户端集合
				for (auto & x : g_clients) {  // 向其他客户端群发添加新用户的消息。
					NewUserJoin userjoin;
					userjoin.sock = _cSock;
					send(x, (const char*)&userjoin, sizeof(NewUserJoin), 0);
				}
				g_clients.push_back(_cSock);
				printf("新客户端加入:socket = %d, IP = %s \n", (int)_cSock, inet_ntoa(clientAddr.sin_addr));
			}
		}
		
		// 寻找触发事件的文件描述符
		for (int n = (int)g_clients.size() - 1; n >= 0; --n){
		    if (FD_ISSET(g_clients[n], &fdRead)){
			// 处理
			if ( -1 == processor(g_clients[n])){

			    auto iter = g_clients.begin() + n; // std::vector<SOCKET>::iterator
			    if (iter != g_clients.end()){
				g_clients.erase(iter);
			    }
			}
		    }
		}


		printf("处理其他主线程业务...");
	}

#ifdef _WIN32
	// 8. 关闭套接字
	for (int n = (int)g_clients.size() - 1; n >= 0; --n) {
		closesocket(g_clients[n]);
	}
	closesocket(_sock);

	// -------------------------
	// Windows网络开发框架
	WSACleanup();
#else
	// 8. 关闭套接字
	for (int n = (int)g_clients.size() - 1; n >= 0; --n) {
		close(g_clients[n]);
	}
	close(_sock);

#endif
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
	int nlen = (int)recv(_cSock, szRecv, sizeof(DataHeader), 0);
	DataHeader * header = (DataHeader*)szRecv;  // 包头指针指向缓冲区
	if (nlen <= 0) {
		printf("client socket = %d quit\n", _cSock);
		return -1;
	}
	// 解析数据头
	//printf("收到命令：%d 数据长度 %d\n", header.cmd, header.dataLength);
	switch (header->cmd)
	{
		case CMD_LOGIN: {
			// 报文头前面已经收到了，这里要进行地址偏移。下同。
			(int)recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
			Login * login = (Login*)szRecv;
			printf("收到命令:CMD_LOGIN, 数据长度:%d, usrname = %s, passwd = %s\n", login->dataLength, login->userName, login->PassWord);
			// 判断用户密码
			LoginResult ret;
			send(_cSock, (char*)&ret, sizeof(LoginResult), 0);
		}break;
		case CMD_LOGOUT:
		{
			(int)recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
			Logout * logout = (Logout*)szRecv;
			printf("收到命令:CMD_LOGIN, 数据长度:%d, usrname = %s\n", logout->dataLength, logout->userName);
			LogoutResult ret;
			send(_cSock, (char*)&ret, sizeof(ret), 0);
		}break;
		default:
			header->cmd = CMD_ERROR;
			header->dataLength = 0;
			send(_cSock, (char*)&header, sizeof(header), 0);
			break;
		}
	return 0;
}
