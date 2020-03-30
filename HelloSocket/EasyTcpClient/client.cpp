#include "EasyTcpClient.hpp"
#include <thread>
using namespace std;
bool g_brun = true;

void cmdThread();// EasyTcpClient *client);

int main() {

	// FD_SETSIZE ��ʾĬ�����select��������
	const int nCount = 1000;  // FD_SETSIZE - 1;
	vector<EasyTcpClient *> clients;

	for (int n = 0; n < nCount; ++n) {
		clients.push_back(new EasyTcpClient());
		clients[n]->initSocket();
	}
	for (int n = 0; n < nCount; ++n) {
		// ������;�˳�
		if (!g_brun) {
			for (auto & x : clients) {
				x->Close();
				delete x;
			}
			clients.clear();
			return 0;
		}
		clients[n]->Connect("127.0.0.1", 4567);
	}

	// �����߳�
	std::thread t1(cmdThread);//, &client);
	t1.detach();  // �� t1 �̺߳͵�ǰ�̷߳��룬������Ҫ���̻߳��ա�

	Login login;
	strcpy(login.userName, "xinyueox");
	strcpy(login.PassWord, "123");
	while (g_brun) {
		for (int n = 0; n < nCount; ++n) {
			clients[n]->SendData(&login);
			// Sleep(1);
		}
		 //if (!client.onRun())
			//break;
	}
	for (auto & x : clients) {
		x->Close();
		delete x;
	}
	clients.clear();
	//client.Close();
	printf("quit\n");
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
			printf("�˳�\n");
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
			printf("δ֪������\n");
		}
	}
}
