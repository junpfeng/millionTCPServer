#define WIN32_LEAN_AND_MEAN  // �����������ڵ��ظ�����
// #pragma comment(lib, "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include<windows.h>
#include<WinSock2.h>
#include <vector>


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

int processor(SOCKET _cSock);
std::vector<SOCKET> g_clients;

int main() {
	// Windows ���翪�����
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);

	// -ͨ�ÿ�ܲ���-
	// 1. ����socket �׽���
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	// 2. ������˿�
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);  // host to net unsigned short
	_sin.sin_addr.S_un.S_addr = INADDR_ANY; // �����������ַ�����Է��ʣ�inet_addr("127.0.0.1");
	if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(_sin))) {
		printf("bind error");
	}
	else {
		printf("bind successfully\n");
	}
	// 3. ��������˿�
	if (SOCKET_ERROR == listen(_sock, 5)) {
		printf("listen error");
	}
	else {
		printf("listen successfully\n");
	}

	while (true) {

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

		for (auto &x : g_clients) {  //����Ҫ�����Ŀͻ���socket�׽��ּ���������ϣ���һ�ν�����û�е�
			FD_SET(x, &fdRead);
		}

		// timeval t = {0,0};  // ������
		int ret = select(_sock + 1, &fdRead, &fdWrite, &fdExp, NULL);  // socket��������
		if (ret < 0) {
			printf("select ����\n");
			break;
		}
		if (FD_ISSET(_sock, &fdRead)) {  // �����_sock�׽��ִ������¼���˵�����µ��������󣬵��� accept
			FD_CLR(_sock, &fdRead);
			sockaddr_in clientAddr = {};
			int nAddrLen = sizeof(clientAddr);
			SOCKET _cSock = INVALID_SOCKET;
			_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);  // ͬ�ͻ��˽�������
			if (INVALID_SOCKET == _cSock) {
				printf("invalid socket\n");
			}
			else {  // �ɹ��󣬽����Ӻõ� �ͻ����׽��ּ��� �ͻ��˼���
				for (auto & x : g_clients) {  // �������ͻ���Ⱥ��������û�����Ϣ��
					NewUserJoin userjoin;
					userjoin.sock = _cSock;
					send(x, (const char*)&userjoin, sizeof(NewUserJoin), 0);
				}
				g_clients.push_back(_cSock);
				printf("�¿ͻ��˼���:socket = %d, IP = %s \n", (int)_cSock, inet_ntoa(clientAddr.sin_addr));
			}
		}

		for (int n = 0; n < fdRead.fd_count; ++n) { // �����ж��¼������еĿͻ��˽��д���
			if (-1 == processor(fdRead.fd_array[n])) {
				auto iter = find(g_clients.begin(), g_clients.end(), fdRead.fd_array[n]);  // ������ɺ󣬴Ӷ��¼��������޳�
				if (iter != g_clients.end())
					g_clients.erase(iter);
			};
		}

		printf("�����������߳�ҵ��...");
	}

	// 8. �ر��׽���
	for (int n = (int)g_clients.size() - 1; n >= 0; --n) {
		closesocket(g_clients[n]);
	}
	closesocket(_sock);

	// -------------------------
	// Windows���翪�����
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
		printf("client socket = %d quit\n", _cSock);
		return -1;
	}
	// ��������ͷ
	//printf("�յ����%d ���ݳ��� %d\n", header.cmd, header.dataLength);
	switch (header->cmd)
	{
		case CMD_LOGIN: {
			// ����ͷǰ���Ѿ��յ��ˣ�����Ҫ���е�ַƫ�ơ���ͬ��
			recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
			Login * login = (Login*)szRecv;
			printf("�յ�����:CMD_LOGIN, ���ݳ���:%d, usrname = %s, passwd = %s\n", login->dataLength, login->userName, login->PassWord);
			// �ж��û�����
			LoginResult ret;
			send(_cSock, (char*)&ret, sizeof(LoginResult), 0);
		}break;
		case CMD_LOGOUT:
		{
			recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
			Logout * logout = (Logout*)szRecv;
			printf("�յ�����:CMD_LOGOUT, ���ݳ���:%d, usrname = %s\n", logout->dataLength, logout->userName);
			LogoutResult ret;
			send(_cSock, (char*)&ret, sizeof(ret), 0);
		}break;
		default:
			header->cmd = CMD_ERROR;
			header->dataLength = 0;
			send(_cSock, (char*)&header, sizeof(header), 0);
			break;
		}
	return 0;
}