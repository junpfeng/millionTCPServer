#ifndef _CELL_THREAD_HPP_
#define _CELL_THREAD_HPP_

#include<functional>
#include"CELLSemaphore.hpp"

class CELLThread {
private:
	// 函数对象封装器，模板参数是函数的格式（返回值和参数列表）
	typedef std::function<void(CELLThread*)> EventCall;
	
public:

	// 线程启动
	// 函数对象本质就是函数指针，因此初值为 nullptr 也是可以的
	void Start(EventCall onCreate = nullptr, 
		EventCall onRun = nullptr,
		EventCall onDestory = nullptr ) {
		// 对在Start和Close的共享数据进行保护：_isRun
		std::lock_guard<std::mutex> lock(_mutex);
		if (false == _isRun) {
			_isRun = true;

			if (onCreate) {
				_onCreate = onCreate;
			}
			if (onRun) {
				_onRun = onRun;
			}
			if (onDestory) {
				_onDestory = onDestory;
			}

			std::thread t(std::mem_fn(&CELLThread::OnWork), this);
			t.detach();
		}
	}
	// 线程关闭
	void Close() {
		std::lock_guard<std::mutex> lock(_mutex);
		if (_isRun) {
			_isRun = false;
			// 阻塞等待线程完全结束
			_sem.wait();
		}
	}
	//在工作函数中退出
	//不需要使用信号量来阻塞等待
	void Exit()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (_isRun)
		{
			_isRun = false;
		}
	}

	// 访问私有成员_isRun
	bool isRun() const {
		return _isRun;
	}
protected:
	// 线程入口函数
	void OnWork() {
		if (_onCreate) {
			// 调用回调函数_onCreate(),参数是CELLThread *,也就是this指针
			_onCreate(this);
		}
		if (_onRun) {
			_onRun(this);
		}
		if (_onDestory) {
			_onDestory(this);
		}
		// 线程结束，唤醒条件变量
		_sem.wakeup();
	}
private:
	// 事件
	EventCall _onCreate;
	EventCall _onRun;
	EventCall _onDestory;
	// 
	std::mutex _mutex;
	// 信号量
	CELLSemaphore _sem;
	
	// 线程是否处于运行态
	bool _isRun = false;
};

#endif  // _CELL_THREAD_HPP_


