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
			break;
		}
		else {
			// 5. �������������������
			send(_sock, cmdBUF, strlen(cmdBUF) + 1, 0);
		}
		// 6.���ܷ�������Ϣ recv
		char recvBuf[256] = {};
		int nlen = recv(_sock, recvBuf, 256, 0);
		if (nlen > 0) {
			// ����ǿת�ǲ���ȫ�ġ�
			DataPackage *dp = (DataPackage*)(recvBuf);
			printf("���յ����ݣ�����%d, ����%s\n", dp->age, dp->name);
		}
		else {
			printf("δ�����յ�����\n");
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