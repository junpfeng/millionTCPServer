// ���շ���Ϣ�ʹ�����Ϣ���з�װ�Ļ���
#ifndef _CELL_TASK_H_
#define _CELL_TASL_H_

#include<thread>
#include <mutex>
#include <list>  // �������������ŵ��Ǳ��ڴ����Ĳ���ɾ��

// �洢�������
class CellTask
{
public:
	CellTask() {

	}

	virtual ~CellTask() {

	}

	// ִ������
	virtual void doTask() {

	}
private:

};

// �����������
class CellTaskServer {
private:
	std::list<CellTask*> _taskList;
	// ������������ ���񻺳���
	std::list<CellTask*> _taskBuf;
	// �����ݻ�������ȡ����ʱ��������
	std::mutex _mutex;
public:
	CellTaskServer()
	{

	}
	virtual ~CellTaskServer()
	{

	}

	// �������
	void addTask(CellTask * task) {
		//_mutex.lock();
		std::lock_guard<std::mutex> lgm(_mutex);
		_taskBuf.push_back(task);
		//_mutex.unlock();
	}

	// �����߳�
	void Start() {

		// �����߳�
		std::thread myThread(std::mem_fn(&CellTaskServer::OnRun), this);
		myThread.detach();
	}

protected:
	// �߳���ں������������߳�
	void OnRun() {
	
		// ���û������
		while (_taskBuf.empty()) {
			// ���� 1 ms�����̳߳صĹ���
			std::chrono::milliseconds t(1);
			std::this_thread::sleep_for(t);
			continue;
		}
		
		// ȡ���������Ĺ������ݣ�����
		{
			std::lock_guard<std::mutex> lgm(_mutex);
			// �ӻ�����ȡ����
			for (auto  pTask : _taskBuf) {
				_taskList.push_back(pTask);
			}
			_taskBuf.clear();
		}

		// �������񣬻���������ص���б����������Բ������ʲ���ͨ�ñ�������
		for (auto & pTask : _taskList) {
			pTask->doTask();
			delete pTask;
		}
		_taskList.clear();
		
	}

};

#endif

