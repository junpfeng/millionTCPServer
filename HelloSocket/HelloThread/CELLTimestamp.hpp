#ifndef _CELLTIMESTAMP_HPP_
#define _CELLTIMESTAMP_HPP_
#include <chrono>  // c++11 ��ʱ��
using namespace std::chrono;

class CELLTimestamp {

public:
	CELLTimestamp() {
		update();
	}
	virtual ~CELLTimestamp() {

	}

	void update() {
		_begin = high_resolution_clock::now();
	}

	// ��ȡ����Ϊ��λ�ģ���ǰʱ���������֮����ʱ��֮��
	double getElapsedTimeSecond() {
		return getElapsedTimeInMicroSec()*0.000001;
	}

	// ��ȡ�Ժ���Ϊ��λ�ģ���ǰʱ���������֮����ʱ��֮��
	double getElapsedTimeInMilliSec() {
		return getElapsedTimeInMicroSec()*0.001;
	}

	// ��ȡ��΢��Ϊ��λ�ģ���ǰʱ���������֮����ʱ��֮��
	long long getElapsedTimeInMicroSec() {
		return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
	}

protected:
	// �ײ�߾���ʱ�����
	time_point<high_resolution_clock> _begin;
};

#endif
