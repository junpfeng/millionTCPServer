#define WIN32_LEAN_AND_MEAN  // �����������ڵ��ظ�����
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include<windows.h>
#include<WinSock2.h>
#include <stdio.h>
#include <thread>  // c++11 �����thread��
// #pragma comment(lib, "ws2_32.lib")

// ����ṹ�����ݽṹ��
// ע��㣺1.�ͻ��˺ͷ�����Ҫ��֤ϵͳλ����ͬ��2.�ֽ�����ͬ
struct DataPackage {
	int age;
	char name[32];
};

// ������ʽ�����ݰ�
enum CMD {  // ������������
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_NEW_USER_JOIN,  // ���û�����
	CMD_ERROR
};
struct DataHeader {  // ��Ϊ�������ݱ��ĵĻ���
	CMD cmd;
	short dataLength;  // ���ݳ���һ�㲻���� 65535
};
struct Login :public DataHeader {  // ��¼
	Login() {
		dataLength = sizeof(Login);
		cmd = CMD_LOGIN;
	}
	char userName[32];
	char PassWord[32];
};
struct LoginResult :public DataHeader {  //��¼���
	LoginResult() {
		dataLength = sizeof(LoginResult);
		cmd = CMD_LOGIN_RESULT;
		result = 0;
	}
	int result;
};
struct Logout :public DataHeader { // �ǳ�
	Logout() {
		dataLength = sizeof(Logout);
		cmd = CMD_LOGOUT;
	}
	char userName[32];
};
struct LogoutResult :public DataHeader {  //�ǳ����
	LogoutResult() {
		dataLength = sizeof(LogoutResult);
		cmd = CMD_LOGOUT_RESULT;
		result = 0;
	}
	int result;
};


// �����¿ͻ��˼���ʱ������ҪȺ���������Ѿ�����Ŀͻ��ˡ�
struct NewUserJoin :public DataHeader {
	NewUserJoin() {
		dataLength = sizeof(NewUserJoin);
		cmd = CMD_NEW_USER_JOIN;
		sock = 0;
	}
	int sock;
};

bool g_brun = true;

int processor(SOCKET _cSock);
void cmdThread(SOCKET _sock);

int main() {
	// Windows ���翪�����
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);

	// -ͨ�ÿ�ܲ���-
	// 1. ����socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == _sock) {
		printf("build socket error\n");
	}

	// 2. ������������
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);
	_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
	if (SOCKET_ERROR == ret) {
		printf("connect error\n");
	}
	else {
		printf("connect sucessfully\n");
	}

	// �����߳�
	std::thread t1(cmdThread, _sock);
	t1.detach();  // �� t1 �̺߳͵�ǰ�̷߳��룬������Ҫ���̻߳��ա�
	while (g_brun) {
		// �ͻ������ select ����ģ��
		fd_set fdReads;
		FD_ZERO(&fdReads);
		FD_SET(_sock, &fdReads);
		timeval t = { 1, 0 };  // ����ʱ������1s
		int ret = select(_sock, &fdReads, NULL, NULL, &t);  
		if (ret < 0) {
			printf("select ����\n");
			break;
		}
		if (FD_ISSET(_sock, &fdReads)) { // �ͻ����յ�����
			FD_CLR(_sock, &fdReads);
			if (-1 == processor(_sock)) {
				printf("�������Ͽ�����\n");
				break;
			}
		}

	}
	// 7. �ر��׽���
	closesocket(_sock);
	// --------------
	// Windows���翪�����
	// ��� socket ����
	WSACleanup();
	printf("quit\n");
	getchar();
	return 0;
}

int processor(SOCKET _cSock)
/*
���룺�ͻ��������׽���
����xxx
���أ��Ƿ���ɹ� -1/0
*/
{

	// ����һ��������
	char szRecv[1024] = { 0 };
	// 5.���ܿͻ�������
	int nlen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
	DataHeader * header = (DataHeader*)szRecv;  // ��ͷָ��ָ�򻺳���
	if (nlen <= 0) {
		printf("server socket = %d quit\n", _cSock);
		return -1;
	}
	// ��������ͷ
	//printf("�յ����%d ���ݳ��� %d\n", header.cmd, header.dataLength);
	switch (header->cmd)
	{
	case CMD_LOGIN_RESULT: {
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		LoginResult * login = (LoginResult  *)szRecv;
		printf("�յ�����:CMD_LOGIN_RESULT, ���ݳ���:%d\n", login->dataLength);
	}break;
	case CMD_LOGOUT_RESULT:
	{
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		LogoutResult * logout = (LogoutResult*)szRecv;
		printf("�յ�����:CMD_LOGOUT_RESULT, ���ݳ���:%d\n", logout->dataLength);
	}break;
	case CMD_NEW_USER_JOIN: {
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		NewUserJoin * userjoin = (NewUserJoin*)szRecv;
		printf("���¿ͻ��˼���:socket = %d, ���ݳ���:%d\n", userjoin->sock, userjoin->dataLength);
	}break;
	default:
		header->cmd = CMD_ERROR;
		header->dataLength = 0;
		send(_cSock, (char*)&header, sizeof(header), 0);
		break;
	}
	return 0;
}

void cmdThread(SOCKET _sock) {
	while (1) {
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit")) {
			printf("�˳�\n");
			g_brun = false;
			return;
		}
		else if (0 == strcmp(cmdBuf, "login")) {
			Login login;
			strcpy(login.userName, "xinyueox");
			strcpy(login.PassWord, "123");
			send(_sock, (const char *)&login, sizeof(Login), 0);
		}
		else if (0 == strcmp(cmdBuf, "logout")) {
			Logout logout;
			strcpy(logout.userName, "xinyueox");
			send(_sock, (const char *)&logout, sizeof(Logout), 0);
		}
		else {
			printf("δ֪������\n");
		}
	}
}
