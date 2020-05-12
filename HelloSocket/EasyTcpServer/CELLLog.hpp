#ifndef _CELL_LOG_HPP_
#define _CELL_LOG_HPP_
#include <iostream>
#include <string>
#include "CELL.hpp"
#include "CELLTask.hpp"
#include <ctime>

// 单例模式的日志系统
// 日志的四种等级：:info、debug、warning、error
// 文件mode：
// 读，r：只读形式打开文件；r+：读写形式打开文件；（文件必须实现存在）
// 写，w：只写形式打开文件，并清空文件；w+：读写形式打开文件，并清空；（文件可以不存在，会自动生成文件）
// 追加，a：追加形式的w；a+：追加形式的w+。所谓的追加是指，写入的内容会被追加到文件中已有的内容之后。
class CELLLog
{
private:
	// 防止编译器自动生成默认构造
	CELLLog() {
		// 由于日志需要使用到文件操作，而文件操作是一种很费时的操作
		// 因此，为日志中的文件IO单独开启一个线程
		
		_taskServer.Start();
	}
	// 防止编译器自动生成默认复制构造
	CELLLog(const CELLLog & other) {
		;
	}

public:
	// 静态方法可以独立于类对象调用，从而创建单例对象
	static CELLLog & Instance() {
		// 局部静态变量的初始化是原子操作
		static CELLLog sLog;
		return sLog;
	}
	~CELLLog() {
		// 关闭线程
		_taskServer.Close();
		// 关闭日志文件
		if (_logFile) {
			Info("CellLog fclose(_logFile)\n");
			fclose(_logFile);
			_logFile = nullptr;
		}
	}
public:
	// 日志等级分为：四级:info、debug、warning、error
	static void Info(const char* pStr)
	{
		CELLLog* pLog = &Instance();
		// 将文件IO相关的操作放到线程内执行
		// 这里采用了 = 复制传参，是因为线程是detach模式，
		// detach模式下，必须使用复制，而不是引用，防止主线程先于子线程结束，导致引用数据错误
		pLog->_taskServer.addTask([=]() {
			if (pLog->_logFile)
			{
				// 使用c标准时间库
				auto t = system_clock::now();
				auto tNow = system_clock::to_time_t(t);
				//fprintf(pLog->_logFile, "%s", ctime(&tNow));
				std::tm* now = std::gmtime(&tNow);
				// fprintf 即 file prinf,即将标准输出重定向到文件
				fprintf(pLog->_logFile, "%s", "Info ");
				// 将时间信息加入日志
				fprintf(pLog->_logFile, "[%d-%d-%d %d:%d:%d]", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
				fprintf(pLog->_logFile, "%s", pStr);
				// 清空 FILE* 指向的流缓冲区
				// 实际上，向文件写入内容，并发是直接写入的，中间会有一级缓冲区（缓冲区的应用在本项目中随处可见）
				// 向文件写入的内容首先放在缓冲区内，当使用fflush后，会主动将缓冲区内容全部写入文件，并清空缓冲区
				// 防止缓冲区占满
				fflush(pLog->_logFile);
			}
			printf("%s", pStr);
		});
	}

	// 变参模板 形式
	template<typename ...Args>
	static void Info(const char * pformat, Args ... args) {
		CELLLog *pLog = &Instance();
		pLog->_taskServer.addTask([=]() {
			if (pLog->_logFile) {
				
				auto t = system_clock::now();
				auto tNow = system_clock::to_time_t(t);

				std::tm* now = std::gmtime(&tNow);
				fprintf(pLog->_logFile, "%s", "Info ");
				fprintf(pLog->_logFile, "[%d-%d-%d %d:%d:%d]", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
				fprintf(pLog->_logFile, pformat, args...);
				fflush(pLog->_logFile);
			}
			printf(pformat, args...);
		});

	}

	// 设置日志文件的路径
	void setLogPath(const char * logPath, const char * mode) {
		// 异常处理，如果日志文件已经存在了，就会先关闭之前的文件
		if (_logFile) {
			Info("Celllog::setLogPath _logFile != nullptr \n");
			fclose(_logFile);
			_logFile = nullptr;
		}

		_logFile = fopen(logPath, mode);
		if (_logFile)
		{
			Info("CELLLog::setLogPath success,<%s,%s>\n", logPath, mode);
		}
		else {
			Info("CELLLog::setLogPath failed,<%s,%s>\n", logPath, mode);
		}
	}
private:
	// 日志存储到文件中去
	FILE * _logFile = nullptr;
	CellTaskServer _taskServer;
};
#endif  // _CELL_LOG_HPP_
