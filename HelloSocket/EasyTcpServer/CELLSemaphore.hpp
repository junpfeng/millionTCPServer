#ifndef _CELL_SEMAPHORE_HPP_
#define _CELL_SEMAPHORE_HPP_

#include<chrono>
#include<thread>
// ��������ģ���ź���
#include <condition_variable> 
 
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
		// �Խ��� lock_guard ��ԭ���ǽ����ļӺͽ��װ������
		// �Ӷ�ʹ�������һ��������������Զ�����
		// �� unique_lock �Ǳ� lock_guard ���ܸ���һЩ��һ��ģ���ࡣ
		std::unique_lock<std::mutex> ulock(_mutex);
		// std::lock_guard<std::mutex> lock(_mutex);
		//if (--_wait < 0) {
		//	// ����ʹ��unique_lock������Ϊ���������Ĳ���Ҫ���
		//	// ��ʱ�ͷ�unlock ����ͬʱ�����ȴ�������������
		//	_cv.wait(ulock, [this]()->bool {
		//		return _wakeup > 0;
		//	});
		//	--_wakeup;
		//}
		// ÿ����������ʹ���ź���--���ź�С�ڵ���0ʱ���߳�����
		if (--_sem_count <= 0) {
			_cv.wait(ulock);
		}
	}

	// �̻߳���
	void wakeup() {
		std::lock_guard<std::mutex> lock(_mutex);  // ���캯�����Զ�����
		//if (++_wait <= 0) {
		//	++_wakeup;
		//	_cv.notify_one();  // ��һ��������������
		//}
		// ÿ�λ��ѣ�����ʹ���ź���++��һ���ź�������0�����̻߳��ѣ�����������
		if (++_sem_count > 0) {
			_cv.notify_one();
		}
	}

private:

	bool _isWaitExit = false;

	std::mutex _mutex;
	// ���� -- �ȴ� ��������
	std::condition_variable _cv;
	
	// �ź��� > 0 ʱ��������С�ڵ��� 0 ʱ����
	int _sem_count = 1;

	// ����
	int _wait = 0;
	int _wakeup = 0;
};


#endif