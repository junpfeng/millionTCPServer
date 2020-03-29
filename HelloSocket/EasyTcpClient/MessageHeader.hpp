#ifndef _MESSAGEHEADER_HPP_
#define _MESSAGEHEADER_HPP_
#ifdef _WIN32
	#pragma comment(lib, "ws2_32.lib")
	#define WIN32_LEAN_AND_MEAN  // �����������ڵ��ظ�����
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#include<windows.h>
	#include<WinSock2.h>
#else
	#include <unistd.h>
	#include <arpa/inet.h>

	#define SOCKET int
	#define INVALID_SOCKET (SOCKET)(~0)
	#define SOCKET_ERROR   (-1)
#endif

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
	DataHeader() {
		dataLength = sizeof(DataHeader);
		cmd = CMD_ERROR;
	}
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
	char dat[928];  // ������䣬�� Login ռ1k�ֽ�
};
struct LoginResult :public DataHeader {  //��¼���
	LoginResult() {
		dataLength = sizeof(LoginResult);
		cmd = CMD_LOGIN_RESULT;
		result = 0;
	}
	int result;
	char data[992];
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



#endif
