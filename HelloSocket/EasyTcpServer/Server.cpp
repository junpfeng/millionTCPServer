#include "EasyTcpServer.hpp"
#include <thread>
bool g_bRun = true;
// ���������һ���ṩ�����رյĶ��߳�
void cmdThread() {
	while (1) {
		char cmdBuf[256];

		scanf("%s", cmdBuf); 
		if (0 == strcmp(cmdBuf, "exit")) {
			g_bRun = false;
			printf("�߳��˳�\n");
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
	server.Listen(10000); // ���������5

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
 