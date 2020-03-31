#include<iostream>
#include<thread>
#include <mutex>
// 原子操作库：c++标准
#include <atomic>
#include "CELLTimestamp.hpp"
using namespace std;

mutex m;
const int tCount = 4;
// unsigned int sum = 0;
// 将基础数据类型封装为原子类模板，再typedef
// atomic<int> sum = 0; // typedef atomic<int> atomic_int;
atomic_int sum = 0;
void workFun(int idx) {
	for (unsigned int i = 0; i < 2000000; ++i) {
		// 
		// lock_guard<mutex> lg(m);
		// m.lock();
		sum++;
		// m.unlock();
	}
	// cout << idx << "thread" << endl;
}

int main() {

	// 创建单个线程
	// thread t1(workFun, 2);

	// 创建线程数组
	thread t2[tCount];
	// CELLTimestamp ctamp = CELLTimestamp();
	CELLTimestamp ctamp;
	for (int i = 0; i < tCount; ++i) {
		t2[i] = thread(workFun, i);
		//t2[i].detach();
	}
	for (int i = 0; i < tCount; ++i) {

		t2[i].join();
	}

	/*for(int i = 0; i < 4; ++i)
		cout << "hello, main thread" << endl;*/
	
	cout << ctamp.getElapsedTimeInMilliSec() << ",sum =" << sum << endl;
	ctamp.update();
	sum = 0;
	for (unsigned int i = 0; i < 8000000; ++i) {
		sum++;
	}
	cout << ctamp.getElapsedTimeInMilliSec() << ",sum =" << sum << endl;
	// t1.join();
	return 0;
}