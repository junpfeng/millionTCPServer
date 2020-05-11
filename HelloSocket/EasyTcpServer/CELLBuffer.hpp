#ifndef _CELL_BUFFER_HPP_
#define _CELL_BUFFER_HPP_
#include "CELL.hpp"

// ʹ�� char * ģ��һ�����У����ǲ��ǻ��Ͷ��У�������ͨ�Ķ���

class CELLBuffer {
public:
	CELLBuffer(int nSize = 8192) {
		_nSize = nSize;
		_pBuff = new char[_nSize];
	}
	~CELLBuffer()
	{
		if (_pBuff) {
			delete[] _pBuff;
			_pBuff = nullptr;
		}
	}
	char * data() {
		return _pBuff;
	}

	bool push(const char * pData, int nLen) {
		// �����ж������pData�Ž�ȥ���᲻�ᳬ���ö��еĴ�С
		if (_nLast + nLen <= _nSize) {
			// ��Ҫ���͵����ݣ����������ͻ�����
			// void *memcpy(void *destin, void *source, unsigned n);
			// destion �� Ŀ���ַ��source �Ǳ����Ƶĵ���
			memcpy(_pBuff + _nLast, pData, nLen);
			// ���¶��еĳ���
			_nLast += nLen;

			// ��������push֮�󣬶�������
			if (_nLast == SEND_BUFF_SIZE) {
				++_fullCount;
			}
			// push �ɹ�
			return true;
		}
		else {
			++_fullCount;
		}
		// pushʧ��
		return false;
	}

	void pop(int nLen) {
		// ���㱻pop֮��_nLast�Ĵ�С
		int n = _nLast - nLen;
		if (n > 0) {
			// ��������Ǩ��
			memcpy(_pBuff, _pBuff + nLen, n);
		}
		// ����_nLast
		_nLast = n;
		// 
		if (_fullCount > 0)
			--_fullCount;
	}

	int write2socket(SOCKET sockfd) {
		int ret = 0;
		// �жϻ������Ƿ������ݣ��Լ������Ķ˿��Ƿ���Ч
		if (_nLast > 0 && INVALID_SOCKET != sockfd) {
			// �������ݣ�ȫ������
			ret = send(sockfd, _pBuff, _nLast, 0);
			// ����β������
			_nLast = 0;
			_fullCount = 0;
		}
		return ret;
	 }
	
	int read4socket(SOCKET sockfd) {
		// �����������Ҫʣ��ռ�
		if (_nSize - _nLast > 0) {
			// ָ��_pBuff �п���ռ����ʼ��ַ
			char * szRecv = _pBuff + _nLast;
			// 
			int nLen = (int)recv(sockfd, szRecv, _nSize - _nLast, 0);
			if (nLen <= 0) {
				return nLen;
			}
			// ���»������Ľ�βָ��
			_nLast += nLen;
			// ���ض�ȡ�����ݳ���
			return nLen;
		}
		// û��ʣ��ռ�
		return 0;
	}

	// �жϻ��������Ƿ����ٴ洢��һ����Ϣͷ����
	bool hasMsg() {
		// �ж���Ϣ�����������ݳ��ȴ�����Ϣͷ
		if (_nLast >= sizeof(netmsg_DataHeader)) {
			// �ӻ�����ȡ��һ����Ϣ
			netmsg_DataHeader * header = (netmsg_DataHeader*)_pBuff;
			// �����������ٴ洢һ����Ϣͷ����
			return _nLast >= header->dataLength;
		}
		return false;
	}
private:
	// �ڶ������������ͻ�����
	char * _pBuff = nullptr;
	// 
	// list <char*> _pBuffList;
	// ���ݽṹ��β��ָ��
	int _nLast = 0;
	// �����������ֽڴ�С
	int _nSize = 0;
	// ������д����������
	int _fullCount = 0;
};

#endif  //_CELL_BUFFER_HPP_