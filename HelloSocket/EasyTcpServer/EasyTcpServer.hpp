#ifndef __EASYTCPSERVER_HPP_
#define __EASYTCPSERVER_HPP_

#ifdef _WIN32
	// #pragma comment(lib, "ws2_32.lib")
	#define WIN32_LEAN_AND_MEAN  // �����������ڵ��ظ�����
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
	#include<windows.h>
	#include<WinSock2.h>
#else
	#include <unistd.h>
	#include <sys/select.h>
	#include <arpa/inet.h>

	#define SOCKET int
	#define INVALID_SOCKET (SOCKET)(~0)
	#define SOCKET_ERROR   (-1)
#endif

#include <vector>
#include <stdio.h>
#include "MessageHeader.hpp"
#include <algorithm>
#include <malloc.h>
std::vector<SOCKET> g_clients;


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
	
public:
	EasyTcpServer():_sock(INVALID_SOCKET)
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
			printf("<socket=%d>close old connection\n", _sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock) {
			printf("build socket failed\n");
		}else {
			printf("build <socket=%d> successfully\n",_sock);
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
		socklen_t nAddrLen = sizeof(clientAddr);
		SOCKET _cSock = INVALID_SOCKET;
		_cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);  // ͬ�ͻ��˽�������
		if (!ValidSocket(_cSock)) {
			printf("invalid socket\n");
			return INVALID_SOCKET;
		}else {  // �ɹ��󣬽����Ӻõ� �ͻ����׽��ּ��� �ͻ��˼���
			for (auto & x : g_clients) {  // �������ͻ���Ⱥ��������û�����Ϣ��
				NewUserJoin userjoin;
				userjoin.sock = _cSock;
				send(x, (const char*)&userjoin, sizeof(NewUserJoin), 0);
			}
			g_clients.push_back(_cSock);
			printf("new joiner:socket = %d, IP = %s \n", (int)_cSock, inet_ntoa(clientAddr.sin_addr));
		}
		return _cSock;
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
		int maxfd = _sock;
		for (auto &x : g_clients) {  //����Ҫ�����Ŀͻ���socket�׽��ּ���������ϣ���һ�ν�����û�е�
			FD_SET(x, &fdRead);  // ���пͻ��˼��ϵ��׽��ֶ�����Ҫ������
			maxfd = std::max(maxfd, x);
		}

		// timeval t = {0,0};  // ������

		int ret = select(maxfd + 1, &fdRead, &fdWrite, &fdExp, NULL);  // socket��������
		if (ret < 0) {
			printf("select error\n");
			return ret;
		}else if (FD_ISSET(_sock, &fdRead)) {  // �����_sock�׽��ִ������¼���˵�����µ��������󣬵��� accept
			FD_CLR(_sock, &fdRead);  // һ��������ĳ���׽��ִ������¼������Ƚ���Ӽ����¼��������������Ϊÿ�ε����������ʱ���Ὣ�����׽�����ӽ�����
			Accept();  // �����µ���������
		}
		// ��ѯ�¼����ϣ�Ѱ�Ҵ����¼��ļ�������
		for (auto &x : g_clients) { 
		    if (!FD_ISSET(x, &fdRead))  // ���ļ�������û�д������¼�����
			continue;
		 
		    DataHeader * header = RecvData(x);
		    if (nullptr == header) {  // ����nullptr����ʾ�ͻ����Ѿ��Ͽ�����
			auto iter = find(g_clients.begin(), g_clients.end(), x);  // ���Ͽ����ӵĿͻ����׽��ִӿͻ��˼������޳���
			if (iter != g_clients.end())
			g_clients.erase(iter);
		    }
		    else { // �������¼�
			ProcessNetMsg(x, header);
			free(header);
		    }
		}
		printf("processing another tasks...");
		return 0;
	}
	
	// ����������
	virtual void ProcessNetMsg(SOCKET _cSock, DataHeader *header) {
		// ��������ͷ
		//printf("�յ����%d ���ݳ��� %d\n", header.cmd, header.dataLength);
		switch (header->cmd)
		{
		case CMD_LOGIN: {	
			Login * login = (Login*)header;
			printf("get cmd :CMD_LOGIN, data length:%d, usrname = %s, passwd = %s\n", login->dataLength, login->userName, login->PassWord);
			// �ж��û����룬ʵ���ϻ�û������
			DataHeader * header = new LoginResult;
			SendData(_cSock, header);
		}break;
		case CMD_LOGOUT:
		{
			Logout * logout = (Logout*)header;
			printf("get cmd:CMD_LOGOUT, data length:%d, usrname = %s\n", logout->dataLength, logout->userName);
			DataHeader * header = new LogoutResult;
			SendData(_cSock, header);
		}break;
		default:
			header->cmd = CMD_ERROR;
			header->dataLength = 0;
			SendData(_cSock, header);
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

	// ��������ͷ
	DataHeader* RecvData(SOCKET _cSock) {
		// ����һ��������
		// char szRecv[1024] = { 0 };
		char * szRecv = (char*)malloc(sizeof(char) * 1024);
		// 5.���ܿͻ�������
		int nlen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
		DataHeader * header = (DataHeader*)szRecv;  // ��ͷָ��ָ�򻺳���
		if (nlen <= 0) {
			printf("client socket = %d quit\n", _cSock);
			free(szRecv);
			return nullptr;
		}
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);  // ���յ������ݲ�δʹ��
		return header;
	}

	// �ر�
	void Close() {
#ifdef _WIN32
		// -------------------------
		// Windows���翪�����
		WSACleanup();
		// 8. �ر�ȫ�ֿͻ����׽��ּ���
		for (int n = (int)g_clients.size() - 1; n >= 0; --n) {
			if (ValidSocket(g_clients[n]))
				closesocket(g_clients[n]);
		}
		if (ValidSocket(_sock))
			closesocket(_sock);  // �رռ����׽���
#else
		// 8. �ر��׽���
		for (int n = (int)g_clients.size() - 1; n >= 0; --n) {
			if (ValidSocket(g_clients[n]))
				close(g_clients[n]);
		}
		if (ValidSocket(_sock))
			close(_sock);
#endif
	}
	// ��������
	// ��ǰIP + �˿��Ƿ����ڹ�����
	bool ValidSocket(SOCKET _sock) {
		return INVALID_SOCKET == _sock ? false : true;
	}
};

#endif
 
