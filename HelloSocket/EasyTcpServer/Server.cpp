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
	//accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
	char msgBUF[] = "hello, lam server.";

	_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
	if (INVALID_SOCKET == _cSock) {
		printf("invalid client socket\n");
	}
	printf("�¿ͻ���IP: = %s \n", inet_ntoa(clientAddr.sin_addr));

	char _recvBUF[128] = {};
	while (true) {
		
		// 5.���ܿͻ�������
		int nlen = recv(_cSock, _recvBUF, 128, 0);
		if (nlen <= 0) {
			printf("client quit\n");
			break;
		}

		printf("���յ�����Ϣ%s\n", _recvBUF);
		// 6.��������
		if (0 == strcmp(_recvBUF, "getInfo")) {
			// 7 send ���ͻ��˷�������
			DataPackage dp = { 80, "junpfeng" };
			// ���ｫ�ṹ��תΪ �ַ��� Ҳ˵���ˣ������ͨ����֮�����ǿת
			send(_cSock, (const char *)&dp, strlen(msgBUF) + 1, 0);
		}else {
			char msgBUF[] = "hello, lam server. what did you send?";
			// 7. ���ͻ��˷�������
			send(_cSock, msgBUF, strlen(msgBUF) + 1, 0);
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