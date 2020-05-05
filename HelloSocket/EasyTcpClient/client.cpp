#include"EasyTcpClient.hpp"
#include"CELLTimestamp.hpp"
#include<thread>
#include<atomic>

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
			printf("ï¿½Ë³ï¿½cmdThreadï¿½ß³ï¿½\n");
			break;
		}
		else {
			printf("ï¿½ï¿½Ö§ï¿½Öµï¿½ï¿½ï¿½ï¿½î¡£\n");
		}
	}
}

//¿Í»§¶ËÊýÁ¿
const int cCount = 8;
//·¢ËÍÏß³ÌÊýÁ¿
const int tCount = 4;
//ï¿½Í»ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
EasyTcpClient* client[cCount];
std::atomic_int sendCount;
std::atomic_int readyCount;

void recvThread(int begin, int end)
{
	while (g_bRun)
	{
		for (int n = begin; n < end; n++)
		{
			client[n]->OnRun();
		}
	}
}

void sendThread(int id)
{
	printf("thread<%d>,start\n", id);
	//4ï¿½ï¿½ï¿½ß³ï¿½ ID 1~4
	int c = cCount / tCount;
	int begin = (id - 1)*c;
	int end = id*c;

	for (int n = begin; n < end; n++)
	{
		client[n] = new EasyTcpClient();
	}
	for (int n = begin; n < end; n++)
	{
		// ¾ÖÓòÍøÍâÍøIP:223.106.134.35
		// ÔÆ·þÎñÆ÷IP47.96.105.148
		// ±¾µØIP 127.0.0.1
		client[n]->Connect("127.0.0.1", 4567);
	}

	printf("thread<%d>,Connect<begin=%d, end=%d>\n", id, begin, end);

	readyCount++;
	while (readyCount < tCount)
	{//ï¿½È´ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ß³ï¿½×¼ï¿½ï¿½ï¿½Ã·ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
		std::chrono::milliseconds t(10);
		std::this_thread::sleep_for(t);
	}
	//
	std::thread t1(recvThread, begin, end);
	t1.detach();
	//
	Login login[10];
	for (int n = 0; n < 10; n++)
	{
		strcpy(login[n].userName, "xinyeox");
		strcpy(login[n].PassWord, "123");
	}
	const int nLen = sizeof(login);
	while (g_bRun)
	{
		for (int n = begin; n < end; n++)
		{
			if (SOCKET_ERROR != client[n]->SendData(login, nLen))
			{
				sendCount++;
			}
		}
		//std::chrono::milliseconds t(10);
		//std::this_thread::sleep_for(t);
	}

	for (int n = begin; n < end; n++)
	{
		client[n]->Close();
		delete client[n];
	}

	printf("thread<%d>,exit\n", id);
}

int main()
{
	//ï¿½ï¿½ï¿½ï¿½UIï¿½ß³ï¿½
	std::thread t1(cmdThread);
	t1.detach();

	//ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ß³ï¿½
	for (int n = 0; n < tCount; n++)
	{
		std::thread t1(sendThread,n+1);
		t1.detach();
	}

	CELLTimestamp tTime;

	while (g_bRun)
	{
		auto t = tTime.getElapsedSecond();
		if (t >= 1.0)
		{
			printf("thread<%d>,clients<%d>,time<%lf>,send<%d>\n",tCount, cCount,t,(int)(sendCount/ t));
			sendCount = 0;
			tTime.update();
		}
#ifdef _WIN32_
		Sleep(1);
#else
		sleep(1);
#endif
	}

	printf("ï¿½ï¿½ï¿½Ë³ï¿½ï¿½ï¿½\n");
	return 0;
}
