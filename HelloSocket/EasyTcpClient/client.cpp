#include "EasyTcpClient.hpp"
#include<thread>

bool g_bRun = true;
void cmdThread()
{
	while (true)
	{
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit"))
		{
			g_bRun = false;
			printf("�˳�cmdThread�߳�\n");
			break;
		}
		else {
			printf("��֧�ֵ����\n");
		}
	}
}

//�ͻ�������
const int cCount = 4000;
//�����߳�����
const int tCount = 4;
// ȫ�ֿͻ�������,���̹߳���
EasyTcpClient* client[cCount];

// id��ʾ�ڼ����߳� id = 1~4
void sendThread(int id)
{
	//4���߳� ID 1~4��ÿ���̴߳����Ŀͻ�������
	int c = cCount / tCount;
	// begin�ǵ�ǰ�߳̿ͻ��˵���ʼ����
	int begin = (id - 1)*c;
	// end�ǵ�ǰ�߳̿ͻ��˵���������
	int end = id * c;

	for (int n = begin; n < end; n++)
	{
		client[n] = new EasyTcpClient();
	}
	for (int n = begin; n < end; n++)
	{
		client[n]->Connect("127.0.0.1", 4567);
	}

	printf("thread<%d>,Connect=%d\n", id);
	std::chrono::milliseconds t(3000);
	std::this_thread::sleep_for(t);
	const int SL = 5;
	Login login[SL];
	for (int n = 0; n < SL; n++)
	{
		strcpy(login[n].userName, "xinyueox");
		strcpy(login[n].PassWord, "123");
	}
	const int nLen = sizeof(login);
	
	while (g_bRun)
	{
		for (int n = begin; n < end; n++)
		{
			client[n]->SendData(login, nLen);
			client[n]->OnRun();
		}
	}

	for (int n = begin; n < end; n++)
	{
		client[n]->Close();
		delete client[n];
	}
}

int main()
{
	//����UI�߳�
	std::thread t1(cmdThread);
	t1.detach();

	//���������߳�
	for (int n = 0; n < tCount; n++)
	{
		std::thread t1(sendThread, n + 1);
		t1.detach();
	}

	while (g_bRun)  // ��ѭ��
		Sleep(100);

	printf("���˳���\n");
	return 0;
}