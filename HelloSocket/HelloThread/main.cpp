#include<iostream>
#include<thread>
#include <mutex>
// ԭ�Ӳ����⣺c++��׼
#include <atomic>
#include "CELLTimestamp.hpp"
using namespace std;

mutex m;
const int tCount = 4;
// unsigned int sum = 0;
// �������������ͷ�װΪԭ����ģ�壬��typedef
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

	// ���������߳�
	// thread t1(workFun, 2);

	// �����߳�����
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