#ifndef _CELL_SEMAPHORE_HPP_
#define _CELL_SEMAPHORE_HPP_

#include<chrono>
#include<thread>
// 条件变量模拟信号量
#include <condition_variable> 
 
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
		// 自解锁 lock_guard 的原理是将锁的加和解封装到类内
		// 从而使得类对象一旦出了作用域就自动解锁
		// 而 unique_lock 是比 lock_guard 功能更多一些的一种模板类。
		std::unique_lock<std::mutex> ulock(_mutex);
		// std::lock_guard<std::mutex> lock(_mutex);
		//if (--_wait < 0) {
		//	// 这里使用unique_lock，是因为条件变量的参数要求的
		//	// 暂时释放unlock 锁，同时阻塞等待条件变量激活
		//	_cv.wait(ulock, [this]()->bool {
		//		return _wakeup > 0;
		//	});
		//	--_wakeup;
		//}
		// 每次阻塞都会使得信号量--，信号小于等于0时，线程阻塞
		if (--_sem_count <= 0) {
			_cv.wait(ulock);
		}
	}

	// 线程唤醒
	void wakeup() {
		std::lock_guard<std::mutex> lock(_mutex);  // 构造函数，自动加锁
		//if (++_wait <= 0) {
		//	++_wakeup;
		//	_cv.notify_one();  // 将一个条件变量激活
		//}
		// 每次唤醒，都会使得信号量++，一旦信号量大于0，则线程唤醒（条件变量）
		if (++_sem_count > 0) {
			_cv.notify_one();
		}
	}

private:

	bool _isWaitExit = false;

	std::mutex _mutex;
	// 阻塞 -- 等待 条件变量
	std::condition_variable _cv;
	
	// 信号量 > 0 时不阻塞，小于等于 0 时阻塞
	int _sem_count = 1;

	// 保留
	int _wait = 0;
	int _wakeup = 0;
};


#endif