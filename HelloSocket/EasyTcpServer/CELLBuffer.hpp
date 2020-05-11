#ifndef _CELL_BUFFER_HPP_
#define _CELL_BUFFER_HPP_
#include "CELL.hpp"

// 使用 char * 模拟一个队列，但是不是环型队列，而是普通的队列

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
		// 首先判断如果将pData放进去，会不会超过该队列的大小
		if (_nLast + nLen <= _nSize) {
			// 将要发送的数据，拷贝到发送缓冲区
			// void *memcpy(void *destin, void *source, unsigned n);
			// destion 是 目标地址，source 是被复制的地区
			memcpy(_pBuff + _nLast, pData, nLen);
			// 更新队列的长度
			_nLast += nLen;

			// 经过上述push之后，队列满了
			if (_nLast == SEND_BUFF_SIZE) {
				++_fullCount;
			}
			// push 成功
			return true;
		}
		else {
			++_fullCount;
		}
		// push失败
		return false;
	}

	void pop(int nLen) {
		// 计算被pop之后，_nLast的大小
		int n = _nLast - nLen;
		if (n > 0) {
			// 整个数据迁移
			memcpy(_pBuff, _pBuff + nLen, n);
		}
		// 更新_nLast
		_nLast = n;
		// 
		if (_fullCount > 0)
			--_fullCount;
	}

	int write2socket(SOCKET sockfd) {
		int ret = 0;
		// 判断缓冲区是否有数据，以及监听的端口是否有效
		if (_nLast > 0 && INVALID_SOCKET != sockfd) {
			// 发送数据：全部发完
			ret = send(sockfd, _pBuff, _nLast, 0);
			// 数据尾部清零
			_nLast = 0;
			_fullCount = 0;
		}
		return ret;
	 }
	
	int read4socket(SOCKET sockfd) {
		// 如果缓冲区还要剩余空间
		if (_nSize - _nLast > 0) {
			// 指向_pBuff 中空余空间的起始地址
			char * szRecv = _pBuff + _nLast;
			// 
			int nLen = (int)recv(sockfd, szRecv, _nSize - _nLast, 0);
			if (nLen <= 0) {
				return nLen;
			}
			// 更新缓冲区的结尾指针
			_nLast += nLen;
			// 返回读取的数据长度
			return nLen;
		}
		// 没有剩余空间
		return 0;
	}

	// 判断缓冲区内是否至少存储了一份消息头数据
	bool hasMsg() {
		// 判断消息缓冲区的数据长度大于消息头
		if (_nLast >= sizeof(netmsg_DataHeader)) {
			// 从缓冲区取出一份消息
			netmsg_DataHeader * header = (netmsg_DataHeader*)_pBuff;
			// 缓冲区内至少存储一份消息头数据
			return _nLast >= header->dataLength;
		}
		return false;
	}
private:
	// 第二缓冲区，发送缓冲区
	char * _pBuff = nullptr;
	// 
	// list <char*> _pBuffList;
	// 数据结构的尾部指针
	int _nLast = 0;
	// 缓冲区的总字节大小
	int _nSize = 0;
	// 缓冲区写满次数计数
	int _fullCount = 0;
};

#endif  //_CELL_BUFFER_HPP_