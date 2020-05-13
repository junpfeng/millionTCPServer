#ifndef _CELL_NET_WORK_HPP_
#define _CELL_NET_WORK_HPP_
#include "CELL.hpp"

// 单例网络类
class CELLNetWork
{
private:
	CELLNetWork() {
#ifdef _WIN32
		//启动Windows socket 2.x环境
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#else
		/*if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		return -1;*/
		// 在linux内，客户端断开可能会导致服务器断开，
		// 是由于SIGPIPE 这个信号触发的，下面将该信号忽略。
		signal(SIGPIPE, SIG_IGN);

#endif
	}
	~CELLNetWork() {
#ifdef _WIN32
		// 清理 Windows socket 环境
		WSACleanup();
#endif
	}
	// 拷贝构造
	CELLNetWork(const CELLNetWork & other) {
		;
	}

public:
	static void Init() {
		static CELLNetWork obj;
	}
};

#endif