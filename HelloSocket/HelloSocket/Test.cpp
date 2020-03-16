#define WIN32_LEAN_AND_MEAN  // 避免引入早期的重复定义

#include<windows.h>
#include<WinSock2.h>

// #pragma comment(lib, "ws2_32.lib")

int main() {
	// Windows 网络开发框架
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);

	// -通用框架部分-



	// --------------

	// Windows网络开发框架
	WSACleanup();
	return 0;
}