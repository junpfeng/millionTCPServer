#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

#ifdef _WIN32
	#define FD_SETSIZE      25060
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#include<windows.h>
	#include<WinSock2.h>
	#pragma comment(lib,"ws2_32.lib")
#else
	#include<unistd.h> //uni std
	#include<arpa/inet.h>
	#include<string.h>

	#define SOCKET int
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
#endif

#include<stdio.h>
#include<vector>
#include <map>
#include<thread>
#include<mutex>
#include<atomic>  // ԭ�Ӳ�����

#include"MessageHeader.hpp"
#include"CELLTimestamp.hpp"  // c++11��ʱ��

//��������С��Ԫ��С
#ifndef RECV_BUFF_SZIE
#define RECV_BUFF_SZIE 10240
#endif 

// Ϊÿ�����ӵĿͻ��˽���һ������������
class ClientSocket
{
public:
	ClientSocket(SOCKET sockfd = INVALID_SOCKET)
	{
		_sockfd = sockfd;
		memset(_szMsgBuf, 0, sizeof(_szMsgBuf));
		_lastPos = 0;
	}

	SOCKET sockfd()
	{
		return _sockfd;
	}

	char* msgBuf()
	{
		return _szMsgBuf;
	}

	int getLastPos()
	{
		return _lastPos;
	}
	void setLastPos(int pos)
	{
		_lastPos = pos;
	}

	//  ��������
	int SendData(DataHeader* header)
	{
		if (header)
		{
			return send(_sockfd, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}

private:
	// socket fd_set  file desc set
	SOCKET _sockfd;
	//�ڶ������� ��Ϣ������
	char _szMsgBuf[RECV_BUFF_SZIE * 5];
	//��Ϣ������������β��λ��
	int _lastPos;
};

class INetEvent
{
public:
	//���麯��
	//�ͻ����뿪�¼�
	virtual void onNetJoin(ClientSocket *pClient) = 0;
	virtual void OnNetLeave(ClientSocket* pClient) = 0;
	virtual void OnNetMsg(ClientSocket * pClient, DataHeader* header) = 0;
private:

};


// ԭ�����߳���ѭ���������еĿͻ������ӣ����ÿ�����ӽ������̣߳�
// Ϊ���̷߳�װһ���������ڴ���ͻ�������
class CellServer
{
public:
	CellServer(SOCKET sock = INVALID_SOCKET)
	{
		_sock = sock;
		_pNetEvent = nullptr;  // 
		_clients_change = true;
	}

	~CellServer()
	{
		Close();
		_sock = INVALID_SOCKET;
	}
	// ��������ڽ������ָ��ͬʵ�ֵ�����
	void setEventObj(INetEvent* event)
	{
		_pNetEvent = event;
	}

	//�ر�Socket
	void Close()
	{
		if (_sock != INVALID_SOCKET)
		{
#ifdef _WIN32
			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{
				closesocket(_clients[n]->sockfd());
				delete _clients[n];
			}
			// 8 �ر��׽���closesocket
			closesocket(_sock);  //���Windows socket�����������߳�ȥ���
#else
			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{
				close(_clients[n]->sockfd());
				delete _clients[n];
			}
			// 8 �ر��׽���closesocket
			close(_sock);
#endif
			_clients.clear();
		}
	}

	//�Ƿ�����
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}


	bool OnRun()
	{
		while (isRun())
		{
			// ���¿ͻ��˼���
			// �ٰѿͻ��˴�_clientsBuffת�Ƶ�_clients,�ӻ��������ȡ���ͻ�����
			if (_clientsBuff.size() > 0)
			{
				_clients_change = true;
				// �Խ���
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pClient : _clientsBuff)
				{
					_clients[pClient->sockfd()] = pClient;
				}
				_clientsBuff.clear();
			}

			//���û����Ҫ����Ŀͻ��ˣ�������
			if (_clients.empty())
			{
				// �߳�����1ms����ֹ���Ͻ�������жϣ�������Դ���˷ѡ�
				// ����c++11�Ķ�ʱ���󣬴���һ��1ms�Ķ���
				std::chrono::milliseconds t(1);
				// ����c++11���̺߳������Ե�ǰ�߳�����1ms
				std::this_thread::sleep_for(t);
				continue;
			}

			//�������׽��� BSD socket
			fd_set fdRead;//��������socket�� ����
						  //������
			FD_ZERO(&fdRead);
			// ֻ�е��ͻ��˼��Ϸ��͸ı�ʱ����������¼������зŶ���
			if (_clients_change) {
				_clients_change = false;
				_maxSock = _clients.begin()->second->sockfd();
				for (auto iter : _clients)
				{
					FD_SET(iter.second->sockfd(), &fdRead);
					if (_maxSock < iter.second->sockfd())
					{
						_maxSock = iter.second->sockfd();
					}
				}
				memcpy(&_fdRead_bak, &fdRead, sizeof(fd_set));
			}
			else {
				memcpy(&fdRead, &_fdRead_bak, sizeof(fd_set));
			}

			///nfds ��һ������ֵ ��ָfd_set����������������(socket)�ķ�Χ������������
			///���������ļ����������ֵ+1 ��Windows�������������д0
			int ret = select(_maxSock + 1, &fdRead, nullptr, nullptr, nullptr);
			if (0 > ret)
			{
				printf("select���������\n");
				Close();
				return false;
			}
			else if (0 == ret) {
				continue;
			}

#ifdef _WIN32
			for (int n = 0; n < fdRead.fd_count; n++)
			{
				auto iter = _clients.find(fdRead.fd_array[n]);
				if (iter != _clients.end())
				{
					if (-1 == RecvData(iter->second))
					{
						if (_pNetEvent)
							_pNetEvent->OnNetLeave(iter->second);
						_clients_change = true;
						_clients.erase(iter->first);
					}
				}
				else {
					printf("error. if (iter != _clients.end())\n");
				}

			}
#else
			std::vector<ClientSocket*> temp;
			for (auto iter : _clients)
			{
				if (FD_ISSET(iter.second->sockfd(), &fdRead))
				{
					if (-1 == RecvData(iter.second))
					{
						if (_pNetEvent)
							_pNetEvent->OnNetLeave(iter.second);
						_clients_change = false;
						temp.push_back(iter.second);
					}
				}
			}
			for (auto pClient : temp)
			{
				_clients.erase(pClient->sockfd());
				delete pClient;
			}
#endif
		}
	}
	//������
	char _szRecv[RECV_BUFF_SZIE] = {};
	//�������� ����ճ�� ��ְ�
	int RecvData(ClientSocket* pClient)
	{
		// 5 ���տͻ�������
		int nLen = (int)recv(pClient->sockfd(), _szRecv, RECV_BUFF_SZIE, 0);
		//printf("nLen=%d\n", nLen);
		if (nLen <= 0)
		{
			printf("�ͻ���<Socket=%d>���˳������������\n", pClient->sockfd());
			return -1;
		}
		//����ȡ�������ݿ�������Ϣ������
		memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);
		//��Ϣ������������β��λ�ú���
		pClient->setLastPos(pClient->getLastPos() + nLen);

		//�ж���Ϣ�����������ݳ��ȴ�����ϢͷDataHeader����
		while (pClient->getLastPos() >= sizeof(DataHeader))
		{
			//��ʱ�Ϳ���֪����ǰ��Ϣ�ĳ���
			DataHeader* header = (DataHeader*)pClient->msgBuf();
			//�ж���Ϣ�����������ݳ��ȴ�����Ϣ����
			if (pClient->getLastPos() >= header->dataLength)
			{
				//��Ϣ������ʣ��δ�������ݵĳ���
				int nSize = pClient->getLastPos() - header->dataLength;
				//����������Ϣ
				OnNetMsg(pClient, header);
				//����Ϣ������ʣ��δ��������ǰ��
				memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, nSize);
				//��Ϣ������������β��λ��ǰ��
				pClient->setLastPos(nSize);
			}
			else {
				//��Ϣ������ʣ�����ݲ���һ��������Ϣ
				break;
			}
		}
		return 0;
	}

	//��Ӧ������Ϣ
	virtual void OnNetMsg(ClientSocket *pClient, DataHeader* header)
	{
		_pNetEvent->OnNetMsg(pClient, header);
		//switch (header->cmd)
		//{
		//case CMD_LOGIN:
		//{

		//	Login* login = (Login*)header;
		//	//printf("�յ��ͻ���<Socket=%d>����CMD_LOGIN,���ݳ��ȣ�%d,userName=%s PassWord=%s\n", cSock, login->dataLength, login->userName, login->PassWord);
		//	//�����ж��û������Ƿ���ȷ�Ĺ���
		//	LoginResult ret;
		//	pClient->SendData(&ret);
		//}
		//break;
		//case CMD_LOGOUT:
		//{
		//	Logout* logout = (Logout*)header;
		//	//printf("�յ��ͻ���<Socket=%d>����CMD_LOGOUT,���ݳ��ȣ�%d,userName=%s \n", cSock, logout->dataLength, logout->userName);
		//	//�����ж��û������Ƿ���ȷ�Ĺ���
		//	LogoutResult ret;
		//	pClient->SendData(&ret);
		//}
		//break;
		//default:
		//{
		//	printf("<socket=%d>�յ�δ������Ϣ,���ݳ��ȣ�%d\n", pClient->sockfd(), header->dataLength);
		//	DataHeader ret;
		//	pClient->SendData(&ret);
		//}
		//break;
		//}
	}

	void addClient(ClientSocket* pClient)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		//_mutex.lock();
		_clientsBuff.push_back(pClient);
		//_mutex.unlock();
	}
	

	// ����������һ���µ��߳�
	void Start()
	{
		// �̵߳���������ڷ�������һ�����صĲ�����thisָ��
		// mem_fn����Ա����תΪ��������ʹ��ָ��������ð�
		_thread = std::thread(std::mem_fn(&CellServer::OnRun), this);
	}

	size_t getClientCount()
	{
		return _clients.size() + _clientsBuff.size();
	}



private:
	// ���_sockֻ��Ϊ�˱�����̼߳������׽������ĸ��������߳�Ŀǰûʲôʵ������
	SOCKET _sock;
	//��ʽ�ͻ�����
	std::map<SOCKET, ClientSocket*> _clients;
	// std::vector<ClientSocket*> _clients;
	//����ͻ�����
	std::vector<ClientSocket*> _clientsBuff;
	// ���������
	std::mutex _mutex;
	// ��ʹ������ָ��֮ǰ��������Ҫʹ��ָ��
	std::thread _thread;
	// �����¼�����
	INetEvent* _pNetEvent;
private:
	// ���ݶ��¼�����
	fd_set _fdRead_bak;
	// ��Ƕ��¼������Ƿ����仯
	bool _clients_change;
	SOCKET _maxSock;
	
};

// ���������̶߳����𣺽������Ӻͷַ��ͻ��ˣ�������һЩͳ���Ե����ݴ���
class EasyTcpServer : public INetEvent
{
private:
	SOCKET _sock;  // �������ļ����׽���
	// ���̶߳����ټ�¼�������ӵĿͻ���
	// ���������̶߳���
	std::vector<CellServer*> _cellServers;
	// ��ʱ
	CELLTimestamp _tTime;
	// ����������INetEventȥ����
	std::atomic_int _recvCount;
	// �ͻ��˼���
	std::atomic_int _clientCount;
public:
	EasyTcpServer()
	{
		_sock = INVALID_SOCKET;
		_recvCount = 0;
		_clientCount = 0;
	}
	virtual ~EasyTcpServer()
	{
		Close();
	}
	//��ʼ��Socket
	SOCKET InitSocket()
	{
#ifdef _WIN32
		//����Windows socket 2.x����
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		if (INVALID_SOCKET != _sock)
		{
			printf("<socket=%d>�رվ�����...\n", (int)_sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			printf("���󣬽���socketʧ��...\n");
		}
		else {
			printf("����socket=<%d>�ɹ�...\n", (int)_sock);
		}
		return _sock;
	}

	//��IP�Ͷ˿ں�
	int Bind(const char* ip, unsigned short port)
	{
		//if (INVALID_SOCKET == _sock)
		//{
		//	InitSocket();
		//}
		// 2 bind �����ڽ��ܿͻ������ӵ�����˿�
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);//host to net unsigned short

#ifdef _WIN32
		if (ip) {
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		}
#else
		if (ip) {
			_sin.sin_addr.s_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.s_addr = INADDR_ANY;
		}
#endif
		int ret = bind(_sock, (sockaddr*)&_sin, sizeof(_sin));
		if (SOCKET_ERROR == ret)
		{
			printf("����,������˿�<%d>ʧ��...\n", port);
		}
		else {
			printf("������˿�<%d>�ɹ�...\n", port);
		}
		return ret;
	}

	//�����˿ں�
	int Listen(int n)
	{
		// 3 listen ��������˿�
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret)
		{
			printf("socket=<%d>����,��������˿�ʧ��...\n", _sock);
		}
		else {
			printf("socket=<%d>��������˿ڳɹ�...\n", _sock);
		}
		return ret;
	}

	//���ܿͻ�������
	SOCKET Accept()
	{
		// 4 accept �ȴ����ܿͻ�������
		sockaddr_in clientAddr = {};
		int nAddrLen = sizeof(sockaddr_in);
		SOCKET cSock = INVALID_SOCKET;
#ifdef _WIN32
		cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
		cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t *)&nAddrLen);
#endif
		if (INVALID_SOCKET == cSock)
		{
			printf("socket=<%d>����,���ܵ���Ч�ͻ���SOCKET...\n", (int)_sock);
		}
		else
		{
			//NewUserJoin userJoin;
			//SendDataToAll(&userJoin);
			// ���¼���Ŀͻ��ˣ��ָ��߳�ȥ����
			addClientToCellServer(new ClientSocket(cSock));
			
			//printf("socket=<%d>�¿ͻ���<%d>���룺socket = %d,IP = %s \n", (int)_sock, _clients.size(),(int)cSock, inet_ntoa(clientAddr.sin_addr));
		}
		return cSock;
	}


	// ���¼���Ŀͻ��ˣ�����Ŀǰ���ٵĵ�һ���߳�ȥ����
	void addClientToCellServer(ClientSocket* pClient)
	{
		// �ͻ���ȫ���������߳�ȥ�������̲߳��ٱ���
		//���ҿͻ��������ٵ�CellServer��Ϣ�������
		auto pMinServer = _cellServers[0];  // ��ʼ��Ϊ��һ�����̶߳���
		for (auto pCellServer : _cellServers)
		{
			if (pMinServer->getClientCount() > pCellServer->getClientCount())
			{
				pMinServer = pCellServer;
			}
		}
		pMinServer->addClient(pClient);
		onNetJoin(pClient);
	}

	void Start(unsigned int threadCount = 1)
	{
		// cell������������Ϊ�߳����
		// ���������̱߳��ݷ��������̶߳���
		for (int n = 0; n < threadCount; n++)
		{
			// ����һ����ν��΢����������ͨ��_sock ��ʶ�ö��������ļ����׽���
			// cellServer�Ƿ��������߳�
			auto ser = new CellServer(_sock);
			// �������̵߳ķ������̳߳�
			_cellServers.push_back(ser);
			// �������ָ��������EasyTcpServer���Ӷ�����EasyTcpServer��д��OnNetMsg����
			// ��֮Ϊע�������¼�
			ser->setEventObj(this);
			// �����߳�
			ser->Start();
		}
	}
	//�ر�Socket
	void Close()
	{
		if (_sock != INVALID_SOCKET)
		{
			// �ͻ��˵Ĺر�ȫ���ŵ����߳�ȥ
#ifdef _WIN32
			
			// 8 �ر��׽���closesocket
			closesocket(_sock);
			//------------
			//���Windows socket����
			WSACleanup();
#else
			// 8 �ر��׽���closesocket
			close(_sock);
#endif
		}
	}
	//����������Ϣ
	//int _nCount = 0;
	bool OnRun()
	{
		if (isRun())
		{
			time4msg();
			//�������׽��� BSD socket
			fd_set fdRead;//��������socket�� ����
						  //fd_set fdWrite;
						  //fd_set fdExp;
						  //������
			FD_ZERO(&fdRead);  //Ϊ�˼ӿ��ٶȣ�ֻ������¼�������ʱ����ʱ����
			//FD_ZERO(&fdWrite);
			//FD_ZERO(&fdExp);
			//����������socket�����뼯��
			FD_SET(_sock, &fdRead);
			//FD_SET(_sock, &fdWrite);
			//FD_SET(_sock, &fdExp);
			///nfds ��һ������ֵ ��ָfd_set����������������(socket)�ķ�Χ������������
			///���������ļ����������ֵ+1 ��Windows�������������д0
			timeval t = { 0,10 };  // �������ʱ��10us
			// windows�ĵ�һ����������Ҫ���������׽��ֵ����ֵ
			int ret = select(_sock + 1, &fdRead, 0, 0, &t); //
															//printf("select ret=%d count=%d\n", ret, _nCount++);
			if (ret < 0)
			{
				printf("select���������\n");
				Close();
				return false;
			}
			//�ж��Ƿ����¿ͻ��˽��룬��������socket���Ƿ��ڼ�����
			if (FD_ISSET(_sock, &fdRead))
			{
				FD_CLR(_sock, &fdRead);
				// Accept()���ɹ�����ʱ����ɵ��������֣�ͬ�ͻ��˽�������
				Accept();
				return true;
			}
			// ���̲߳�����ѯ���ĸ��ͻ��˴������¼�������һ�����¿ͻ��ˣ��ͽ��¿ͻ��˷ַ������̴߳���
			return true;
		}
		return false;
	}
	//�Ƿ�����
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}

	//��¼ÿ�봦�����Ϣ����
	void time4msg()
	{
		auto t1 = _tTime.getElapsedTimeSecond();
		// ÿ��ȥһ�룬���һ�ν��ܵ���Ϣ����Ϣ
		if (t1 >= 1.0)
		{
			// �ڼ����̣߳�ʱ�䣬�������ļ����׽��֣��ͻ��˵�������ÿ�������߳��ܹ���������ݰ�����ÿһ����¼��Ϣ�壬����һ�Σ���
			printf("thread<%d>,time<%lf>,socket<%d>,clients<%d>,recvCount<%d>\n", _cellServers.size(), t1, _sock, (int)_clientCount, (int)(_recvCount / t1));
			_recvCount = 0;
			_tTime.update();
		}
	}

	//����ָ��Socket����


	// Ⱥ���Ŀ��ܼ���û��
	//void SendDataToAll(DataHeader* header)
	//{
	//	for (int n = (int)_clients.size() - 1; n >= 0; n--)
	//	{
	//		_clients[n]->SendData(header);
	//	}
	//}

	// �� pclient ��ĳ���̼߳����Ŀͻ��˼������Ƴ�
	virtual void OnNetLeave(ClientSocket* pClient)
	{
		// ���̲߳���������ӵĿͻ���
		_clientCount--;

	}
	virtual void OnNetMsg(ClientSocket * pClient, DataHeader* header)
	{
		_recvCount++;
	}
	virtual void onNetJoin(ClientSocket *pClient) {
		_clientCount++;
	}
};

#endif // !_EasyTcpServer_hpp_
