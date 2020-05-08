#ifndef _CELL_SEMAPHORE_HPP_
#define _CELL_SEMAPHORE_HPP_

#include<chrono>
#include<thread>

// 自定义一个信号量对象
class CELLSemaphore {
public:
	CELLSemaphore() {

	}

	~CELLSemaphore()
	{

	}
	// 线程阻塞
	void wait() {
		// 这种模拟信号量的手段非常的不安全
		// 比如:多线程时，wakeup先于wait调用，那么wait就拥有无法解除循环
		_isWaitExit = true;
		while (_isWaitExit) {
			std::chrono::microseconds t(1);
			std::this_thread::sleep_for(t);
		}
	}

	// 线程唤醒
	void wakeup() {
		_isWaitExit = false;

	}

private:
	bool _isWaitExit = false;
};


#endif