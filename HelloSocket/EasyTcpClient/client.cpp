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
			printf("退出cmdThread线程\n");
			break;
		}
		else {
			printf("不支持的命令。\n");
		}
	}
}

//客户端数量
const int cCount = 4000;
//发送线程数量
const int tCount = 4;
// 全局客户端数组,多线程共享
EasyTcpClient* client[cCount];

// id表示第几个线程 id = 1~4
void sendThread(int id)
{
	//4个线程 ID 1~4，每个线程创建的客户端数量
	int c = cCount / tCount;
	// begin是当前线程客户端的起始数量
	int begin = (id - 1)*c;
	// end是当前线程客户端的最终数量
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
	//启动UI线程
	std::thread t1(cmdThread);
	t1.detach();

	//启动发送线程
	for (int n = 0; n < tCount; n++)
	{
		std::thread t1(sendThread, n + 1);
		t1.detach();
	}

	while (g_bRun)  // 死循环
		Sleep(100);

	printf("已退出。\n");
	return 0;
}