#include "EasyTcpServer.hpp"

int main() {
	EasyTcpServer server;
	server.initSocket();
	server.Bind(nullptr, 4567);
	server.Listen(5); // 最长监听队列5

	while (true) {
		server.WaitNetMsg();
	}

	server.Close();
	printf("quit\n");
	getchar();
	return 0;
}
 