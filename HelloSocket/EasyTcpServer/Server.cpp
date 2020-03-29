#include "EasyTcpServer.hpp"
#include <thread>
bool g_bRun = true;
// 服务端设置一个提供主动关闭的多线程
void cmdThread() {
	while (1) {
		char cmdBuf[256];

		scanf("%s", cmdBuf); 
		if (0 == strcmp(cmdBuf, "exit")) {
			g_bRun = false;
			printf("线程退出\n");
			return;
		}
		else {
			printf("command not support\n");
		}
	}
}

int main() {
	EasyTcpServer server;
	server.initSocket();
	server.Bind(nullptr, 4567);
	server.Listen(1000); // 最长监听队列5

	std::thread t1(cmdThread);
	t1.detach();
	while (g_bRun) {
		server.WaitNetMsg();
	}

	server.Close();
	printf("quit\n");
	getchar();
	return 0;
}
 
