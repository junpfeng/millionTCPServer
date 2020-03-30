#ifndef _EASYTCPCLIENT_HPP_
#define _EASYTCPCLIENT_HPP_
#ifdef _WIN32
	// #pragma comment(lib, "ws2_32.lib")
	#define FD_SETSIZE 1024
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
#include <vector>
#include "MessageHeader.hpp"

std::vector<SOCKET> g_clients;
// �����������ھ���Ĵ�С��Ϊʲô��ֱ���õڶ������������ں˻����������ݣ�
// ����Ϊ�ڶ������������ݿ��ܻ�û�����ֱ꣬�ӽ��ܻ��и��ǵķ��գ���˲����ھ�����н�
const unsigned int UNIT_BUFF_SZIE = 10240; // 10 kb 
// �ڶ��������Ĵ�С
const unsigned int RECV_BUFF_SZIE = 102400; // 100 kb

class EasyTcpClient
{
	SOCKET _sock;
	char _unRecv[UNIT_BUFF_SZIE];
	char _RecvBuf[RECV_BUFF_SZIE];
	int _lastPos;  // ָ���ڶ����������Ѿ�ʹ�ó���
public:
	EasyTcpClient() :_sock(INVALID_SOCKET), _unRecv{  }, _RecvBuf{  }, _lastPos(0) 
	{
		// ��ʼ��socket

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
		// 5.���ܿͻ�������
		// �Ƚ�������ͷ
		int nlen = recv(_cSock, _unRecv, UNIT_BUFF_SZIE, 0);
		if (nlen <= 0) {
			printf("server socket = %d quit\n", _cSock);
			return -1;
		}
		// �����ݷŵ��ڶ�������
		memcpy(_RecvBuf + _lastPos, _unRecv, nlen);
		_lastPos += nlen; 
		// �жϵڶ��������Ƿ��������İ�ͷ����
		while (_lastPos >= sizeof(DataHeader)) {
			DataHeader * header = (DataHeader*)_RecvBuf;  // ��ͷָ��ָ�򻺳���
			if (_lastPos >= header->dataLength) {
				// �ڶ���������ʣ����Ϣ����ǰ����
				int RemainSize = _lastPos - header->dataLength;
				OnNetMsg(header);
				memcpy(_RecvBuf, _unRecv + header->dataLength, RemainSize);
				_lastPos = RemainSize;
			}
			else {
				// the remain msg not enough
				break;
			}
		}
		return 0;
	}

	// ��Ӧ������Ϣ
	void OnNetMsg(DataHeader *header) {
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
			case CMD_ERROR: {
				printf("<socket=%d>recv:CMD_ERROR, data length:%d\n", _sock, header->dataLength);
			}break;
			default:
				//header->cmd = CMD_ERROR;
				//header->dataLength = 0;
				//send(_sock, (char*)&header, sizeof(header), 0);
				printf("δ������Ϣ\n");
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