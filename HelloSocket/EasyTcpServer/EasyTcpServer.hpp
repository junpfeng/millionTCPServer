#ifndef __EASYTCPSERVER_HPP_
#define __EASYTCPSERVER_HPP_

#ifdef _WIN32
	// #pragma comment(lib, "ws2_32.lib")
	#define FD_SETSIZE 10240  // ��������꣬�޸���select������׽���
	#define WIN32_LEAN_AND_MEAN  // �����������ڵ��ظ�����
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
	#include<windows.h>
	#include<WinSock2.h>
#else
	#include <unistd.h>
	#include <arpa/inet.h>

	#define SOCKET int
	#define INVALID_SOCKET (SOCKET)(~0)
	#define SOCKET_ERROR   (-1)
#endif

#include <vector>
#include <stdio.h>
#include "MessageHeader.hpp"
#include "CELLTimestamp.hpp"

// �����������ھ���Ĵ�С��Ϊʲô��ֱ���õڶ������������ں˻����������ݣ�
// ����Ϊ�ڶ������������ݿ��ܻ�û�����ֱ꣬�ӽ��ܻ��и��ǵķ��գ���˲����ھ�����н�
const unsigned int RECV_BUFF_SZIE = 10240; // 10 kb 
// �ڶ��������Ĵ�С
const unsigned int MSG_BUFF_SZIE = 102400; // 100 kb

class clientSocket {
private:
	SOCKET _cSock;  // ÿ���ͻ��������࣬ʹ�� _cSock ����ĸ��ͻ���
	char _MsgBuf[MSG_BUFF_SZIE];
	unsigned long long _lastPos;  // ָ���ڶ����������Ѿ�ʹ�ó���

public:
	clientSocket(SOCKET cSock = INVALID_SOCKET) :_cSock(cSock), 
		_lastPos(0), _MsgBuf{} 
	{}
	SOCKET getcSock() {
		return _cSock;
	}
	char * getMsgBuf() {
		return _MsgBuf;
	}

	int getLastPos() {
		return _lastPos;
	}

	void setLastPos(int pos) {
		_lastPos = pos;
	}
};

class EasyTcpServer
{
	/*
	* 1.��ʼ���׽��֣������������׽��֣�
	* 2.�󶨶˿�
	* 3.�����˿�
	* 4.IO���ô������Ӻ�����
	* 5.�ر����д򿪵�socket
	* ����1��Ϊʲô������3.�����˿ڲ�ʹ��IO���ü����أ�
	*		����Ϊ���μ����˿�ʱ��û������������Ҫ������ʹ����סҲû��ϵ����
	*/
private:
	SOCKET _sock;
	std::vector<clientSocket *> _clients;
	char _RecvBuf[RECV_BUFF_SZIE];
	CELLTimestamp _tTime;
	unsigned int _recvCount;
public:
	EasyTcpServer() :_sock(INVALID_SOCKET), _RecvBuf{}
	{

	}
	virtual ~EasyTcpServer() {

	}

	// ��ʼ��
	void initSocket() {
#ifdef _WIN32
		// Windows ���翪�����
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		// -ͨ�ÿ�ܲ���-
		// 1. ����socket �׽���
		if (ValidSocket(_sock)) {
			printf("<socket=%d>�رվ�����\n", _sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock) {
			printf("����socketʧ��\n");
		}else {
			printf("����<socket=%d>�ɹ�\n");
		}
	}

	// �󶨶˿�
	int Bind(const char * ip, unsigned short port) {
		// �쳣�����һ��
		unsigned long IP = INADDR_ANY;
		if (nullptr != ip) {
			IP = inet_addr(ip);
		}	
		// 2. ������˿�
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);  // host to net unsigned short
#ifdef _WIN32
		_sin.sin_addr.S_un.S_addr = IP; // �����������ַ�����Է��ʣ�inet_addr("127.0.0.1");
#else
		_sin.sin_addr.s_addr = IP;
#endif 
		if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(_sin))) {
			printf("bind <port=%d> error", port);
			return SOCKET_ERROR;
		}
		else {
			printf("bind <port=%d>successfully\n", port);
			return 0;
		}
	}
	// �����˿�
	int Listen(unsigned int n) {  // ֧�����n���ļ�������
		// 3. ��������˿�
		if (SOCKET_ERROR == listen(_sock, n)) {
			printf("listen<socket=%d> error\n", _sock);
			return SOCKET_ERROR;
		}
		else {
			printf("listen <socket = %d> successfully\n", _sock);
			return 0;
		}
	}

	// ���ܿͻ��˵���������
	SOCKET Accept() {
		sockaddr_in clientAddr = {};
		int nAddrLen = sizeof(clientAddr);
		SOCKET _cSock = INVALID_SOCKET;
		_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);  // ͬ�ͻ��˽�������
		if (!ValidSocket(_cSock)) {
			printf("invalid socket\n");
			return INVALID_SOCKET;
		}else {  
			NewUserJoin userjoin;
			//SendDataToAll(userjoin);  // ���¿ͻ��˼������Ϣ��Ⱥ����ȥ
			_clients.push_back(new clientSocket(_cSock));
			printf("new joiner<number %d> :socket = %d, IP = %s \n", _clients.size(), (int)_cSock, inet_ntoa(clientAddr.sin_addr));
		}
		return _cSock;
	}
	// �����пͻ���Ⱥ����Ϣ���������õĸ�������
	void SendDataToAll(DataHeader & header) {
		for (int n = (int)_clients.size() - 1; n >= 0; --n) {
			SendData(_clients[n]->getcSock(), &header);
		}
	}
	// ��������
	int WaitNetMsg() {
		// selectģ��
		fd_set fdRead;
		fd_set fdWrite;
		fd_set fdExp;
		// ��ʼ���¼�����������
		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExp);
		// ����¼�
		FD_SET(_sock, &fdRead);  //��������socket������¼�������
		FD_SET(_sock, &fdWrite);
		FD_SET(_sock, &fdExp);
		SOCKET maxSock = _sock;
		for (auto &x : _clients) {  //����Ҫ�����Ŀͻ���socket�׽��ּ���������ϣ���һ�ν�����û�е�
			FD_SET(x->getcSock(), &fdRead);  // ���пͻ��˼��ϵ��׽��ֶ�����Ҫ������
			if (maxSock < x->getcSock())
				maxSock = x->getcSock();
		}

		timeval t = {1,0};  // ������
		int ret = select(_sock + 1, &fdRead, &fdWrite, &fdExp, &t);  // ������������¼�1s
		if (ret < 0) {
			printf("select ����\n");
			return ret;
		}else if (FD_ISSET(_sock, &fdRead)) {  // �����_sock�׽��ִ������¼���˵�����µ��������󣬵��� accept
			FD_CLR(_sock, &fdRead);  // һ��������ĳ���׽��ִ������¼������Ƚ���Ӽ����¼��������������Ϊÿ�ε����������ʱ���Ὣ�����׽�����ӽ�����
			Accept();  // �����µ���������
		}

		// ��Ȼ�¼���Ŀͻ����Ѿ������������б��У����ǿͻ������Ƚ������ӵģ������пͻ��˽���������֮ǰ��û�з���������Ϣ��
		for (int n = (int)_clients.size() - 1; n >= 0; --n) {
			// ��ѯ�ж��ǲ��ǵ�ǰsocket��ɵ��¼�����
			if (FD_ISSET(_clients[n]->getcSock(), &fdRead)) {
				// �쳣�жϣ��ͻ��˶Ͽ�
				if (-1 == RecvData(_clients[n]))
				{
					auto iter = _clients.begin() + n;
					if (iter != _clients.end()) {
						delete _clients[n];
						_clients.erase(iter);
						
					}
				}
			}
		}
		// printf("�����������߳�ҵ��...\n");
		return 0;
	}

	// ����������
	virtual void ProcessNetMsg(SOCKET _cSock, DataHeader *header) {
		// ÿ�ε��øú�������ʾ������һ������
		_recvCount++;
		auto t1 = _tTime.getElapsedTimeSecond();
		// ÿ��һ�룬��¼һ��
		if (t1 >= 1.0) {
			printf("socket<%lf>, socket<%d>,_recvCount<%d>, connectedNum<%d>\n", t1, _sock, _recvCount, _clients.size());
			_recvCount = 0;
			_tTime.update();
		}

		// ��������ͷ
		//printf("�յ����%d ���ݳ��� %d\n", header.cmd, header.dataLength);
		switch (header->cmd)
		{
		case CMD_LOGIN: {	
			//Login * login = (Login*)header;
			//printf("�յ�����:CMD_LOGIN, ���ݳ���:%d, usrname = %s, passwd = %s\n", login->dataLength, login->userName, login->PassWord);
			// �ж��û����룬ʵ���ϻ�û������
			//DataHeader * header = new LoginResult;
			//SendData(_cSock, header);
		}break;
		case CMD_LOGOUT:
		{
			//Logout * logout = (Logout*)header;
			//printf("�յ�����:CMD_LOGOUT, ���ݳ���:%d, usrname = %s\n", logout->dataLength, logout->userName);
			//DataHeader * header = new LogoutResult;
			//SendData(_cSock, header);
		}break;
		default:
			printf("<socket = %d>�յ�δ��������\n", _cSock);
			break;
		}
	}
	// ��������
	int SendData(SOCKET _cSock, DataHeader * header) {
		if (INVALID_SOCKET == _cSock || header == nullptr)
		{
			return SOCKET_ERROR;
		}
		return send(_cSock, (const char*) header, header->dataLength, 0);
	}


	int RecvData(clientSocket * cSock) {
		// 5.���ܿͻ�������
		// �����ݴ��ں˻�����ȡ��
		int nlen = recv(cSock->getcSock(), _RecvBuf, RECV_BUFF_SZIE, 0);
		if (nlen <= 0) {
			printf("server socket = %d quit\n", cSock->getcSock());
			return -1;
		}
		// �����ݷŵ��ڶ�������
		memcpy(cSock->getMsgBuf() + cSock->getLastPos(), _RecvBuf, nlen);
		cSock->setLastPos(cSock->getLastPos() + nlen);
		// �жϵڶ��������Ƿ��������İ�ͷ����
		while (cSock->getLastPos() >= sizeof(DataHeader)) {
			DataHeader * header = (DataHeader*)cSock->getMsgBuf();  // ��ͷָ��ָ�򻺳���
			if (cSock->getLastPos() >= header->dataLength) {
				// �ڶ���������ʣ����Ϣ����ǰ����
				int RemainSize = cSock->getLastPos() - header->dataLength;
				while (RemainSize < 0) {
					int a = 1;
				}
				ProcessNetMsg(cSock->getcSock(), header);
				memcpy(cSock->getMsgBuf(), _RecvBuf + header->dataLength, RemainSize);
				cSock->setLastPos(RemainSize);
			}
			else {
				// the  remain msg not enough
				break;
			}
		}
	}

	// �ر�
	void Close() {
#ifdef _WIN32
		// -------------------------
		// Windows���翪�����
		WSACleanup();
		// 8. �ر�ȫ�ֿͻ����׽��ּ���
		for (int n = (int)_clients.size() - 1; n >= 0; --n) {
			if (ValidSocket(_clients[n]->getcSock())) {
				closesocket(_clients[n]->getcSock()); // ��Ϊsocketֻ�Ǹ����֣����Լ�ʹֻ�ǵõ��ķ���ֵ��Ҳ���Թر�
				delete _clients[n];
			}
		}
		if (ValidSocket(_sock))
			closesocket(_sock);  // �رռ����׽���
#else
		// 8. �ر��׽���
		for (int n = (int)_clients.size() - 1; n >= 0; --n) {
			if (ValidSocket(_clients[n]->getcSock())) {
				close(_clients[n]->getcSock());
				delete _clients[n];
			}
		}
		if (ValidSocket(_sock))
			close(_sock);
#endif
		_clients.clear();
	}
	// ��������
	// ��ǰIP + �˿��Ƿ����ڹ�����
	bool ValidSocket(SOCKET _sock) {
		return INVALID_SOCKET == _sock ? false : true;
	}
};

#endif
 