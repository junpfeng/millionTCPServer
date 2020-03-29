#ifndef _CELLTIMESTAMP_HPP_
#define _CELLTIMESTAMP_HPP_
#include <chrono>  // c++11 定时器
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

	// 获取以秒为单位的，当前时间与对象建立之初的时间之差
	double getElapsedTimeSecond() {
		return getElapsedTimeInMicroSec()*0.000001;
	}

	// 获取以毫秒为单位的，当前时间与对象建立之初的时间之差
	double getElapsedTimeInMilliSec() {
		return getElapsedTimeInMicroSec()*0.001;
	}

	// 获取以微秒为单位的，当前时间与对象建立之初的时间之差
	long long getElapsedTimeInMicroSec() {
		return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
	}

protected:
	// 底层高精度时间变量
	time_point<high_resolution_clock> _begin;
};

#endif
