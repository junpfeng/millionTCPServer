#ifndef _CELL_LOG_HPP_
#define _CELL_LOG_HPP_
#include <iostream>
#include <string>
#include "CELL.hpp"
#include "CELLTask.hpp"
#include <ctime>

// ����ģʽ����־ϵͳ
// ��־�����ֵȼ���:info��debug��warning��error
// �ļ�mode��
// ����r��ֻ����ʽ���ļ���r+����д��ʽ���ļ������ļ�����ʵ�ִ��ڣ�
// д��w��ֻд��ʽ���ļ���������ļ���w+����д��ʽ���ļ�������գ����ļ����Բ����ڣ����Զ������ļ���
// ׷�ӣ�a��׷����ʽ��w��a+��׷����ʽ��w+����ν��׷����ָ��д������ݻᱻ׷�ӵ��ļ������е�����֮��
class CELLLog
{
private:
	// ��ֹ�������Զ�����Ĭ�Ϲ���
	CELLLog() {
		// ������־��Ҫʹ�õ��ļ����������ļ�������һ�ֺܷ�ʱ�Ĳ���
		// ��ˣ�Ϊ��־�е��ļ�IO��������һ���߳�
		
		_taskServer.Start();
	}
	// ��ֹ�������Զ�����Ĭ�ϸ��ƹ���
	CELLLog(const CELLLog & other) {
		;
	}

public:
	// ��̬�������Զ������������ã��Ӷ�������������
	static CELLLog & Instance() {
		// �ֲ���̬�����ĳ�ʼ����ԭ�Ӳ���
		static CELLLog sLog;
		return sLog;
	}
	~CELLLog() {
		// �ر��߳�
		_taskServer.Close();
		// �ر���־�ļ�
		if (_logFile) {
			Info("CellLog fclose(_logFile)\n");
			fclose(_logFile);
			_logFile = nullptr;
		}
	}
public:
	// ��־�ȼ���Ϊ���ļ�:info��debug��warning��error
	static void Info(const char* pStr)
	{
		CELLLog* pLog = &Instance();
		// ���ļ�IO��صĲ����ŵ��߳���ִ��
		// ��������� = ���ƴ��Σ�����Ϊ�߳���detachģʽ��
		// detachģʽ�£�����ʹ�ø��ƣ����������ã���ֹ���߳��������߳̽����������������ݴ���
		pLog->_taskServer.addTask([=]() {
			if (pLog->_logFile)
			{
				// ʹ��c��׼ʱ���
				auto t = system_clock::now();
				auto tNow = system_clock::to_time_t(t);
				//fprintf(pLog->_logFile, "%s", ctime(&tNow));
				std::tm* now = std::gmtime(&tNow);
				// fprintf �� file prinf,������׼����ض����ļ�
				fprintf(pLog->_logFile, "%s", "Info ");
				// ��ʱ����Ϣ������־
				fprintf(pLog->_logFile, "[%d-%d-%d %d:%d:%d]", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
				fprintf(pLog->_logFile, "%s", pStr);
				// ��� FILE* ָ�����������
				// ʵ���ϣ����ļ�д�����ݣ�������ֱ��д��ģ��м����һ������������������Ӧ���ڱ���Ŀ���洦�ɼ���
				// ���ļ�д����������ȷ��ڻ������ڣ���ʹ��fflush�󣬻�����������������ȫ��д���ļ�������ջ�����
				// ��ֹ������ռ��
				fflush(pLog->_logFile);
			}
			printf("%s", pStr);
		});
	}

	// ���ģ�� ��ʽ
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

	// ������־�ļ���·��
	void setLogPath(const char * logPath, const char * mode) {
		// �쳣���������־�ļ��Ѿ������ˣ��ͻ��ȹر�֮ǰ���ļ�
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
	// ��־�洢���ļ���ȥ
	FILE * _logFile = nullptr;
	CellTaskServer _taskServer;
};
#endif  // _CELL_LOG_HPP_
