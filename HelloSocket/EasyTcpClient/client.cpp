#include "EasyTcpClient.hpp"
#include <thread>

bool g_brun = true;

void cmdThread();// EasyTcpClient *client);

int main() {

	// FD_SETSIZE 表示默认最大select监听数量
	const int nCount = FD_SETSIZE - 1;
	EasyTcpClient *client[nCount];

	for (int n = 0; n < nCount; ++n) {
		client[n] = new EasyTcpClient();
		client[n]->initSocket();
		client[n]->Connect("127.0.0.1", 4567);
	}

	// 启动线程
	std::thread t1(cmdThread);//, &client);
	t1.detach();  // 将 t1 线程和当前线程分离，即不需要主线程回收。

	Login login;
	strcpy(login.userName, "xinyueox");
	strcpy(login.PassWord, "123");
	while (g_brun) {
		for (int n = 0; n < nCount; ++n) {
			
			client[n]->SendData(&login);
		}
		 //if (!client.onRun())
			//break;
	}
	for (int n = 0; n < nCount; ++n) {

		client[n]->Close();
	}
	//client.Close();
	printf("quit\n");
	getchar();
	return 0;
}

void cmdThread() {//EasyTcpClient *client) {
	while (1) {
		/*Login login;
		sizeof(Login);
		client->SendData(&login);
		Sleep(1);*/

		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit")) {
			printf("退出\n");
			g_brun = false;
			return;
		}
		//else if (0 == strcmp(cmdBuf, "login")) {
		//	Login login;
		//	strcpy(login.userName, "xinyueox");
		//	strcpy(login.PassWord, "123");
		//	client->SendData(&login);
		//}
		//else if (0 == strcmp(cmdBuf, "logout")) {
		//	Logout logout;
		//	strcpy(logout.userName, "xinyueox");
		//	client->SendData((DataHeader*)&logout);
		//}
		else {
			printf("未知的命令\n");
		}
	}
}
