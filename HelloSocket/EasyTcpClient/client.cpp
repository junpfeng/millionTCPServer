#include"EasyTcpClient.hpp"
#include"CELLTimestamp.hpp"
#include<thread>
#include<atomic>

// 自定义个人的客户端，重写一下纯虚函数
class MyClient : public EasyTcpClient {
public:
	virtual void OnNetMsg(netmsg_DataHeader * header) {
		switch (header->cmd) {
		case CMD_LOGIN_RESULT: {
			netmsg_LoginR *login = (netmsg_LoginR*)header;
		}
							   break;
		case CMD_LOGOUT_RESULT: {
			netmsg_LoginR * logout = (netmsg_LoginR*)header;
		}
								break;
		case CMD_NEW_USER_JOIN: {
			netmsg_NewUserJoin* userJoin = (netmsg_NewUserJoin*)header;
		}break;
		case CMD_ERROR: {
			CELLLog::Info("<socket = %d> recv msgType : CMD_ERROR\n", (int)_pClient->sockfd());
		}break;
		default: {
			CELLLog::Info("error, <socket = %d> recv undefine msgType\n", (int)_pClient->sockfd());
		}
		}
	}
};

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
const int cCount = 10000;
//发送线程数量
const int tCount = 40;
//锟酵伙拷锟斤拷锟斤拷锟斤拷
EasyTcpClient* client[cCount];
std::atomic_int sendCount(0);
std::atomic_int readyCount(0);

void recvThread(int begin, int end)
{
	CELLTimestamp t;
	while (g_bRun)
	{
		for (int n = begin; n < end; n++)
		{
			//if (t.getElapsedSecond() > 3.0 && n == begin)
			//	continue;
			client[n]->OnRun();
		}
	}
}

void sendThread(int id)
{
	// printf("thread<%d>,start\n", id);
	CELLLog::Info("thread<%d>, start\n", id);
	//4锟斤拷锟竭筹拷 ID 1~4
	int c = cCount / tCount;
	int begin = (id - 1)*c;
	int end = id*c;

	for (int n = begin; n < end; n++)
	{
		client[n] = new MyClient();
	}
	for (int n = begin; n < end; n++)
	{ 
		// 局域网外网IP:223.106.134.35
		// 云服务器IP47.96.105.148
		// 本地IP 127.0.0.1
// 		client[n]->Connect("47.96.105.148", 4567);
		client[n]->Connect("127.0.0.1", 4567);
	}

	// printf("thread<%d>,Connect<begin=%d, end=%d>\n", id, begin, end);
	CELLLog::Info("thread<%d>,Connect<begin=%d, end=%d>\n", id, begin, end);

	readyCount++;
	while (readyCount < tCount)
	{//锟饺达拷锟斤拷锟斤拷锟竭筹拷准锟斤拷锟矫凤拷锟斤拷锟斤拷锟斤拷
		std::chrono::milliseconds t(100);
		std::this_thread::sleep_for(t);
	}
	//
	std::thread t1(recvThread, begin, end);
	t1.detach();
	//
	const int num_login = 1;
	netmsg_Login login[num_login];
	for (int n = 0; n < num_login; n++)
	{
		strcpy(login[n].userName, "xinyeox");
		strcpy(login[n].PassWord, "123");
	}
	const int nLen = sizeof(login);
	while (g_bRun)
	{
		for (int n = begin; n < end; n++)
		{
			if (SOCKET_ERROR != client[n]->SendData(login))
			{
				sendCount++;
			}
		}
		std::chrono::milliseconds t(99);
		std::this_thread::sleep_for(t);
	}

	for (int n = begin; n < end; n++)
	{
		client[n]->Close();
		delete client[n];
	}

	CELLLog::Info("thread<%d>,exit\n", id);
	// printf("thread<%d>,exit\n", id);
}

int main()
{
	CELLLog::Instance().setLogPath("clientLog.txt", "w");
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
			// printf("thread<%d>,clients<%d>,time<%lf>,send<%d>\n",tCount, cCount,t,(int)(sendCount/ t));
			CELLLog::Info("thread<%d>,clients<%d>,time<%lf>,send<%d>\n", tCount, cCount, t, (int)(sendCount / t));
			sendCount = 0;
			tTime.update();
		}
#ifdef _WIN32
		Sleep(1);
#else
		sleep(1);
#endif
	}

	// printf("客户端完成退出\n");
	CELLLog::Info("客户端完成退出\n");
	return 0;
}
