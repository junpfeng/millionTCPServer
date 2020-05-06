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
			printf("cmdThread线程退出\n");
			break;
		}
		else {
			printf("指令不支持。\n");
		}
	}
}

//客户端数量
const int cCount = 8;
//发送线程数量
const int tCount = 4;
//锟酵伙拷锟斤拷锟斤拷锟斤拷
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
	//4锟斤拷锟竭筹拷 ID 1~4
	int c = cCount / tCount;
	int begin = (id - 1)*c;
	int end = id*c;

	for (int n = begin; n < end; n++)
	{
		client[n] = new EasyTcpClient();
	}
	for (int n = begin; n < end; n++)
	{
		// 局域网外网IP:223.106.134.35
		// 云服务器IP47.96.105.148
		// 本地IP 127.0.0.1
// 		client[n]->Connect("47.96.105.148", 4567);
		client[n]->Connect("127.0.0.1", 4567);
	}

	printf("thread<%d>,Connect<begin=%d, end=%d>\n", id, begin, end);

	readyCount++;
	while (readyCount < tCount)
	{//锟饺达拷锟斤拷锟斤拷锟竭筹拷准锟斤拷锟矫凤拷锟斤拷锟斤拷锟斤拷
		std::chrono::milliseconds t(10);
		std::this_thread::sleep_for(t);
	}
	//
	std::thread t1(recvThread, begin, end);
	t1.detach();
	//
	netmsg_Login login[10];
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
	//锟斤拷锟斤拷UI锟竭筹拷
	std::thread t1(cmdThread);
	t1.detach();

	//锟斤拷锟斤拷锟斤拷锟斤拷锟竭筹拷
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
#ifdef _WIN32
		Sleep(1);
#else
		sleep(1);
#endif
	}

	printf("锟斤拷锟剿筹拷锟斤拷\n");
	return 0;
}
