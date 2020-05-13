#ifndef _CELL_NET_WORK_HPP_
#define _CELL_NET_WORK_HPP_
#include "CELL.hpp"

// ����������
class CELLNetWork
{
private:
	CELLNetWork() {
#ifdef _WIN32
		//����Windows socket 2.x����
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#else
		/*if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		return -1;*/
		// ��linux�ڣ��ͻ��˶Ͽ����ܻᵼ�·������Ͽ���
		// ������SIGPIPE ����źŴ����ģ����潫���źź��ԡ�
		signal(SIGPIPE, SIG_IGN);

#endif
	}
	~CELLNetWork() {
#ifdef _WIN32
		// ���� Windows socket ����
		WSACleanup();
#endif
	}
	// ��������
	CELLNetWork(const CELLNetWork & other) {
		;
	}

public:
	static void Init() {
		static CELLNetWork obj;
	}
};

#endif