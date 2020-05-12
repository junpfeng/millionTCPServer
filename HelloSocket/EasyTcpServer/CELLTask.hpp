#ifndef _CELL_TASK_H_
#define _CELL_TASK_H_

#include<thread>
#include<mutex>
#include<list>

#include<functional>

#include"CellThread.hpp"

//执行任务的服务器
class CellTaskServer 
{
public:
	// 所属server的id
	int serverId = -1;
private:
	typedef std::function<void()> CellTask;
	//任务数据
	std::list<CellTask> _tasks;
	//任务数据缓冲区
	std::list<CellTask> _tasksBuf;
	//改变数据缓冲区时需要加锁
	std::mutex _mutex;
	// 线程管理对象
	CELLThread _thread;
public:
	//添加任务
	void addTask(CellTask task)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_tasksBuf.push_back(task);
	}
	//启动工作线程
	void Start()
	{
		_thread.Start(nullptr, [this](CELLThread* pThread) {
			OnRun(pThread);
		});
	}

	void Close() {
		// CELLLog::Info("CELLTaskServer %d close\n", serverId);
		_thread.Close();
	}

protected:
	//工作函数
	void OnRun(CELLThread *pThread)
	{
		while (pThread->isRun())
		{
			//从缓冲区取出数据
			if (!_tasksBuf.empty())
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pTask : _tasksBuf)
				{
					_tasks.push_back(pTask);
				}
				_tasksBuf.clear();
			}
			//如果没有任务，就等一会
			if (_tasks.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}
			//处理任务
			for (auto pTask : _tasks)
			{
				pTask();
			}
			//清空任务
			_tasks.clear();

		}
		// 防止线程结束时，缓冲区还有任务，处理缓存区内的任务
		for (auto pTask : _tasksBuf)
		{
			pTask();
		}
		//清空任务
		_tasks.clear();
		// CELLLog::Info("CellTaskServer%d.OnRun\n", serverId);
	}
};
#endif // !_CELL_TASK_H_
