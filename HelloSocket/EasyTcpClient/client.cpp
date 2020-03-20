#define WIN32_LEAN_AND_MEAN  // �����������ڵ��ظ�����
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include<windows.h>
#include<WinSock2.h>
#include <stdio.h>
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

	
	while (true) {

		// 3. ������������
		char cmdBUF[128] = {};
		scanf("%s", cmdBUF);
		// 4. ������������
		if (0 == strcmp(cmdBUF, "exit")) {
			printf("exit\n");
			break;
		}
		else if(0 == strcmp(cmdBUF, "login")){
			Login login;
			strcpy(login.userName, "junpfeng");
			strcpy(login.PassWord, "123");
			// 5. ����������������¼
			send(_sock, (const char*)&login, sizeof(login), 0);

			LoginResult loginRet;
			// ���ܷ������ĵ�¼���
			recv(_sock, (char*)&loginRet, sizeof(loginRet), 0);
			printf("loginResult %d \n", loginRet.result);  
		}
		else if (0 == strcmp(cmdBUF, "logout")) {
			Logout logout;
			strcpy(logout.userName, "junpfeng");
			// ����ܷ��������ص�����
			send(_sock, (const char*)&logout, sizeof(logout), 0);
			// ���շ��������ص�����
			LogoutResult logoutRet;
			recv(_sock, (char*)&logoutRet, sizeof(logoutRet), 0);
			printf("logoutResult :%d\n", logoutRet.result);
		}
		else {
			printf("���֧��\n");
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