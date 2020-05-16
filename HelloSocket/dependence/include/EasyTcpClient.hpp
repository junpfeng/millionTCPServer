#ifndef _EasyTcpClient_hpp_
#define _EasyTcpClient_hpp_

#include "CELL.hpp"
#include "CELLNetWork.hpp"
#include "CELLClient.hpp"
#include "MessageHeader.hpp"

class EasyTcpClient
{
//	SOCKET _sock;
//	bool _isConnect;
public:
	EasyTcpClient()
	{
//		_sock = INVALID_SOCKET;
		_isConnect = false;
	}
	
	virtual ~EasyTcpClient()
	{
		Close();
	}
	//初始化socket
	void InitSocket()
	{
		CELLNetWork::Init();

		if (_pClient) {
			CELLLog::Info("关闭就连接 socket = %d\n", (int)_pClient->sockfd());
			Close();
		}
		// 创建一个客户端的套接字（还未绑定和监听）
		SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == sock) {
			CELLLog::Info("创建socket失败\n");
		}
		else {
			// 将创建好的socket 添加到客户端对象中统一管理
			_pClient = new CellClient(sock);
		}
	}

	//连接服务器
	int Connect(const char* ip,unsigned short port)
	{
		// 如果socket还没被创建，则先去创建
		if (!_pClient)
		{
			InitSocket();
		}
		// 2 连接服务器 connect
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
#ifdef _WIN32
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else
		_sin.sin_addr.s_addr = inet_addr(ip);
#endif
		//printf("<socket=%d>正在连接服务器<%s:%d>...\n", _sock, ip, port);
		int ret = connect(_pClient->sockfd(), (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret)
		{
			CELLLog::Info("<socket=%d>错误，连接服务器<%s:%d>失败...\n",(int)_pClient->sockfd(), ip, port);
		}
		else {
			_isConnect = true;
			//printf("<socket=%d>连接服务器<%s:%d>成功...\n",_sock, ip, port);
		}
		return ret;
	}

	//关闭套节字closesocket
	void Close()
	{
		if (_pClient) {
			delete _pClient;
			_pClient = nullptr;
		}
		_isConnect = false;
	}

	//处理网络消息
	bool OnRun()
	{
		if (isRun())
		{
			SOCKET _sock = _pClient->sockfd();
			// 可读事件就是创建的socket
			fd_set fdRead;
			FD_ZERO(&fdRead);
			FD_SET(_sock, &fdRead);
			// 可写事件初始化为无，由 select 决定
			fd_set fdWrite;
			FD_ZERO(&fdWrite);
			// 最长等待 1 ms
			timeval t = { 0,1 };
			int ret = 0;

			// 如果发送缓冲区有数据，才会对发送事件进行监听
			if (_pClient->needWrite())
			{
				FD_SET(_sock, &fdWrite);
				ret = select(_sock + 1, &fdRead, &fdWrite, nullptr, &t);
			}
			else {
				ret = select(_sock + 1, &fdRead, nullptr, nullptr, &t);
			}

			if (ret < 0)
			{
				CELLLog::Info("<socket=%d>select任务结束1\n", (int)_sock);
				Close();
				return false;
			}

			// 可读，即接收数据
			if (FD_ISSET(_sock, &fdRead))
			{
				if (-1 == _pClient->SendDataReal())
				{
					CELLLog::Info("<socket=%d>select任务结束2\n", _sock);
					Close();
					return false;
				}
			}

			// 可写，即发送数据
			if (FD_ISSET(_sock, &fdWrite)) {
				if (-1 == _pClient->SendDataReal()) {
					CELLLog::Info("error, <so cket = %d>OnRun.select SendDataReal exit\n", (int)_sock);
					Close();
					return false;

				}
			}
			return true;
		}
		return false;
	}

	//是否工作中
	bool isRun()
	{
		return _pClient && _isConnect;
	}

	//接收数据 处理粘包 拆分包
	int RecvData(SOCKET cSock)
	{
		if (isRun()) {
			// 5 接收数据
			int nLen = _pClient->RecvData();
			if (nLen > 0) {
				//
				while (_pClient->hasMsg()) {
					// 处理队首数据
					OnNetMsg(_pClient->front_msg());
					// 弹出被处理过的数据
					_pClient->pop_front_msg();
				}
				return nLen;
			}
		}
	}

	//响应网络消息,纯虚函数
	virtual void OnNetMsg(netmsg_DataHeader* header) = 0;

	//发送数据
	int SendData(netmsg_DataHeader* header)
	{
		return _pClient->SendData(header);
	}

	int SendData(const char *pData, int len) {
		return _pClient->SendData(pData, len);
	}
protected:
	CellClient * _pClient = nullptr;
	bool _isConnect = false;
};

#endif