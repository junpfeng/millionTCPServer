// 对收发消息和处理消息进行封装的基类
#ifndef _CELL_TASK_H_
#define _CELL_TASL_H_

#include<thread>
#include <mutex>
#include <list>  // 用作任务链表：优点是便于大量的插入删除

// 存储任务的类
class CellTask
{
public:
	CellTask() {

	}

	virtual ~CellTask() {

	}

	// 执行任务
	virtual void doTask() {

	}
private:

};

// 处理任务的类
class CellTaskServer {
private:
	std::list<CellTask*> _taskList;
	// 生产者消费者 任务缓冲区
	std::list<CellTask*> _taskBuf;
	// 从数据缓冲区存取数据时，加锁。
	std::mutex _mutex;
public:
	CellTaskServer()
	{

	}
	virtual ~CellTaskServer()
	{

	}

	// 添加任务
	void addTask(CellTask * task) {
		//_mutex.lock();
		std::lock_guard<std::mutex> lgm(_mutex);
		_taskBuf.push_back(task);
		//_mutex.unlock();
	}

	// 启动线程
	void Start() {

		// 启动线程
		std::thread myThread(std::mem_fn(&CellTaskServer::OnRun), this);
		myThread.detach();
	}

protected:
	// 线程入口函数：消费者线程
	void OnRun() {
	
		// 如果没有任务
		while (_taskBuf.empty()) {
			// 挂起 1 ms：即线程池的挂起
			std::chrono::milliseconds t(1);
			std::this_thread::sleep_for(t);
			continue;
		}
		
		// 取出缓冲区的共享数据，加锁
		{
			std::lock_guard<std::mutex> lgm(_mutex);
			// 从缓冲区取数据
			for (auto  pTask : _taskBuf) {
				_taskList.push_back(pTask);
			}
			_taskBuf.clear();
		}

		// 处理任务，基于链表的特点进行遍历，兼容性不够，故采用通用遍历方法
		for (auto & pTask : _taskList) {
			pTask->doTask();
			delete pTask;
		}
		_taskList.clear();
		
	}

};

#endif

