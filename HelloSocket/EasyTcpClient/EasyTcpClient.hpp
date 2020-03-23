#ifndef _EASYTCPCLIENT_HPP_
#define _EASYTCPCLIENT_HPP_
#ifdef _WIN32
	// #pragma comment(lib, "ws2_32.lib")
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

#include <stdio.h>
#include "MessageHeader.hpp"
 
class EasyTcpClient
{
	SOCKET _sock;
public:
	EasyTcpClient()
	{
		_sock = INVALID_SOCKET;  // ��ʼ��socket
	}

	// ����������
	virtual ~EasyTcpClient() 
	{
		Close();
	}

	// ��ʼ��socket
	void initSocket()
	{
#ifdef _WIN32
		// Windows ���翪�����
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		// 1. ����socket
		if (INVALID_SOCKET != _sock) {  // ����Ѿ�ָ��һ���򿪵��ļ�����������ر�֮
			printf("close old socket: %d\n", _sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (INVALID_SOCKET == _sock) {
			printf("build socket error\n");
		}
		else {
			printf("build socket successfully\n");
		}
		return;
	}

	// ���ӷ�����
	int Connect(const char *ip, unsigned short port)
	{
		// 2. ������������
		if (INVALID_SOCKET == _sock) {
			initSocket();
		}
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret) {
			printf("connect error\n");
		}
		else {
			printf("connect sucessfully\n");
		}
		return ret;
	}

	// �ر� socket
	void Close()
	{
		if (_sock == INVALID_SOCKET)
			return;
#ifdef _WIN32
		// 7. �ر��׽���
		closesocket(_sock);
		// --------------
		// Windows���翪�����
		// ��� socket ����
		WSACleanup();
#else
		close(_sock);
#endif
	}

	// ��������
	int SendData(DataHeader * header) {
		if (isRun() && header) {  //_sock��Ч��header��Ч
			return send(_sock, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}

	// �������ݣ�����ճ�������
	int RecvData(SOCKET _cSock) {
		// ����һ��������
		char szRecv[1024] = { 0 };
		// 5.���ܿͻ�������
		// �Ƚ�������ͷ
		int nlen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
		DataHeader * header = (DataHeader*)szRecv;  // ��ͷָ��ָ�򻺳���
		if (nlen <= 0) {
			printf("server socket = %d quit\n", _cSock);
			return -1;
		}
		// �ٽ�����������
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		// �Խ��յ������ݣ����з���
		OnNetMsg(_cSock, header);  
		return 0;
	}

	// ��Ӧ������Ϣ
	void OnNetMsg(SOCKET _cSock, DataHeader *header) {
		// ��������ͷ
		//printf("�յ����%d ���ݳ��� %d\n", header.cmd, header.dataLength);
		switch (header->cmd)
		{
		case CMD_LOGIN_RESULT: {

			LoginResult * login = (LoginResult  *)header;
			printf("�յ�����:CMD_LOGIN_RESULT, ���ݳ���:%d\n", header->dataLength);
		}break;
		case CMD_LOGOUT_RESULT:
		{

			LogoutResult * logout = (LogoutResult*)header;
			printf("�յ�����:CMD_LOGOUT_RESULT, ���ݳ���:%d\n", header->dataLength);
		}break;
		case CMD_NEW_USER_JOIN: {
	
			NewUserJoin * userjoin = (NewUserJoin*)header;
			printf("���¿ͻ��˼���:socket = %d, ���ݳ���:%d\n", userjoin->sock, header->dataLength);
		}break;
		default:
			header->cmd = CMD_ERROR;
			header->dataLength = 0;
			send(_cSock, (char*)&header, sizeof(header), 0);
			break;
		}
	}

	// ��������
	bool onRun() {

		// �ж� socket����Ч��
		if (!isRun())
			return false;

		// �ͻ������ select ����ģ��
		fd_set fdReads;
		FD_ZERO(&fdReads);
		FD_SET(_sock, &fdReads);
		timeval t = { 1, 0 };  // ����ʱ������1s
		int ret = select(_sock, &fdReads, NULL, NULL, &t);
		if (ret < 0) {
			printf("<socket=%d> select ����\n", _sock);
			return false;
		}
		if (FD_ISSET(_sock, &fdReads)) { // �ͻ����յ�����
			FD_CLR(_sock, &fdReads);
			// �������¼��������ͽ�������
			if (-1 == RecvData(_sock)) {
				printf("<socket=%d> �������Ͽ�����\n", _sock);
				return false;
			}
		}
		return true;
	}
	//
	bool isRun() {
		return _sock == INVALID_SOCKET ? false : true;
	}
};


#endif