#define WIN32_LEAN_AND_MEAN  // �����������ڵ��ظ�����
// #pragma comment(lib, "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include<windows.h>
#include<WinSock2.h>


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
	
	// 4. �����ȴ��ͻ�������
	sockaddr_in clientAddr = {};
	int nAddrLen = sizeof(sockaddr_in);
	SOCKET _cSock = INVALID_SOCKET;
	char msgBUF[] = "hello, lam server.";

	_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
	if (INVALID_SOCKET == _cSock) {
		printf("invalid client socket\n");
	}
	printf("�¿ͻ���IP: = %s \n", inet_ntoa(clientAddr.sin_addr));

	while (true) {
		DataHeader header;
		// 5.���ܿͻ�������
		int nlen = recv(_cSock, (char*)&header, sizeof(header), 0);
		if (nlen <= 0) {
			printf("client quit\n");
			break;
		}
		// ��������ͷ
		//printf("�յ����%d ���ݳ��� %d\n", header.cmd, header.dataLength);
		switch (header.cmd)
		{
			case CMD_LOGIN: {
				Login login;
				// ����ͷǰ���Ѿ��յ��ˣ�����Ҫ���е�ַƫ�ơ���ͬ��
				recv(_cSock, (char*)&login + sizeof(header), sizeof(Login) - sizeof(header), 0);
				printf("�յ�����:CMD_LOGIN, ���ݳ���:%d, usrname = %s, passwd = %s\n", login.dataLength, login.userName, login.PassWord);
				// �ж��û�����
				LoginResult ret; 
				send(_cSock, (char*)&ret, sizeof(LoginResult), 0);
			}
			break;
			case CMD_LOGOUT: 
			{
				Logout logout;
				recv(_cSock, (char*)&logout + sizeof(header), sizeof(Logout) - sizeof(header), 0);
				LogoutResult ret;
				send(_cSock, (char*)&ret, sizeof(ret), 0);
			}break;
			default:
				header.cmd = CMD_ERROR;
				header.dataLength = 0;
				send(_cSock, (char*)&header, sizeof(header), 0);
				break;
		}
	}

	// 8. �ر��׽���
	closesocket(_sock);

	// -------------------------
	// Windows���翪�����
	WSACleanup();
	printf("quit\n");
	getchar();
	return 0;
}