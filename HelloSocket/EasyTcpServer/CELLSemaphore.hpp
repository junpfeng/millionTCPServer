#ifndef _CELL_SEMAPHORE_HPP_
#define _CELL_SEMAPHORE_HPP_

#include<chrono>
#include<thread>

// �Զ���һ���ź�������
class CELLSemaphore {
public:
	CELLSemaphore() {

	}

	~CELLSemaphore()
	{

	}
	// �߳�����
	void wait() {
		// ����ģ���ź������ֶηǳ��Ĳ���ȫ
		// ����:���߳�ʱ��wakeup����wait���ã���ôwait��ӵ���޷����ѭ��
		_isWaitExit = true;
		while (_isWaitExit) {
			std::chrono::microseconds t(1);
			std::this_thread::sleep_for(t);
		}
	}

	// �̻߳���
	void wakeup() {
		_isWaitExit = false;

	}

private:
	bool _isWaitExit = false;
};


#endif