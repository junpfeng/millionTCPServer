#define WIN32_LEAN_AND_MEAN  // �����������ڵ��ظ�����

#include<windows.h>
#include<WinSock2.h>

// #pragma comment(lib, "ws2_32.lib")

int main() {
	// Windows ���翪�����
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);

	// -ͨ�ÿ�ܲ���-



	// --------------

	// Windows���翪�����
	WSACleanup();
	return 0;
}