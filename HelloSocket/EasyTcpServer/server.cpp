﻿#include "EasyTcpServer.hpp"
#include "CELLMsgStream .hpp"

class MyServer : public EasyTcpServer
{
public:

	//只会被一个线程触发 安全
	virtual void OnNetJoin(CellClient* pClient)
	{
		EasyTcpServer::OnNetJoin(pClient);
	}
	//cellServer 4 多个线程触发 不安全
	//如果只开启1个cellServer就是安全的
	virtual void OnNetLeave(CellClient* pClient)
	{
		EasyTcpServer::OnNetLeave(pClient);
	}
	//cellServer 4 多个线程触发 不安全
	//如果只开启1个cellServer就是安全的
	virtual void OnNetMsg(CellServer* pCellServer, CellClient* pClient, netmsg_DataHeader* header)
	{
		EasyTcpServer::OnNetMsg(pCellServer, pClient, header);
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			// 每当服务器收到一次客户端的消息，就重置心跳计数
			pClient->resetDTHeart();
			//send recv 
			netmsg_Login* login = (netmsg_Login*)header;
			//CELLLog::Info("收到客户端<Socket=%d>请求：CMD_LOGIN,数据长度：%d,userName=%s PassWord=%s\n", cSock, login->dataLength, login->userName, login->PassWord);
			//忽略判断用户密码是否正确的过程
			netmsg_LoginR ret;
			if (SOCKET_ERROR == pClient->SendData(&ret)) {
				CELLLog::Info("socket = %d, SendFull\n", pClient->sockfd());
			}
			//netmsg_LoginR* ret = new netmsg_LoginR();
			//pCellServer->addSendTask(pClient, ret);
		}//接收 消息---处理 发送   生产者 数据缓冲区  消费者 
		break;
		case CMD_LOGOUT:
		{
			netmsg_Logout* logout = (netmsg_Logout*)header;
			// CELLReadStream r(header);
			//
			// r.ReadInt16();
			// netmsg_Logout* logout = (netmsg_Logout*)header;
			//CELLLog::Info("收到客户端<Socket=%d>请求：CMD_LOGOUT,数据长度：%d,userName=%s \n", cSock, logout->dataLength, logout->userName);
			//忽略判断用户密码是否正确的过程
			//netmsg_LogoutR ret;
			//SendData(cSock, &ret);
		}
		break;
		case CMD_C2S_HEART:  // 接收到来自客户端的心跳包
		{
			pClient->resetDTHeart();
			netmsg_s2c_Heart ret;  // 反馈给客户端心跳包
			pClient->SendData(&ret);
		}
		default:
		{
			CELLLog::Info("<socket=%d>收到未定义消息,数据长度：%d\n", pClient->sockfd(), header->dataLength);
		}
		break;
		}
	}
private:

};

int main()
{
	CELLLog::Instance().setLogPath("serverLog.txt", "w");
	MyServer server;
	server.InitSocket();
	server.Bind(nullptr, 4567);
	server.Listen(5);
	server.Start(4);  // 收发客户端的服务器对象线程
	
	// 循环检测输入
	while (true)
	{
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit"))
		{
			server.Close();
			CELLLog::Info("退出cmdThread线程\n");
			break;
		}
		else {
			CELLLog::Info("不支持的命令。\n");
		}
	}

	// server.Close();
	CELLLog::Info("已退出。\n");
	getchar();
	getchar();
	return 0;
}
