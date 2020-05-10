#ifndef _CellClient_hpp_
#define _CellClient_hpp_

#include"CELL.hpp"

//客户端心跳检测死亡计时时间，单位是毫秒
#define CLIENT_HREAT_DEAD_TIME 60000  // 60s

//发送数据定时时间，单位是毫秒，
// 定时发送数据的机制类似于帧同步
#define CLIENT_SEND_BUFF_TIME 200  // 200ms发一次

//服务器存储的客户端对象
class CellClient
{
public:
	int id = -1;
	int serverId = -1;
public:
	CellClient(SOCKET sockfd = INVALID_SOCKET)
	{
		static int n = 1;
		id = n++;

		_sockfd = sockfd;
		memset(_szMsgBuf, 0, RECV_BUFF_SZIE);
		_lastPos = 0;

		memset(_szSendBuf, 0, SEND_BUFF_SZIE);
		_lastSendPos = 0;

		resetDTHeart();
		resetDTSend();
	}
	~CellClient() {
		printf("~CellClient id = %d, serverID = %d\n", id, serverId);
		if (INVALID_SOCKET != _sockfd) {
#ifdef _WIN32
			closesocket(_sockfd);
#else
			close(_sockfd);
#endif
			_sockfd = SOCKET_ERROR;
		}
	}
	SOCKET sockfd()
	{
		return _sockfd;
	}

	char* msgBuf()
	{
		return _szMsgBuf;
	}

	int getLastPos()
	{
		return _lastPos;
	}
	void setLastPos(int pos)
	{
		_lastPos = pos;
	}

	//发送数据
	int SendData(netmsg_DataHeader* header)
	{
		int ret = SOCKET_ERROR;
		//要发送的数据长度
		int nSendLen = header->dataLength;
		//要发送的数据
		const char* pSendData = (const char*)header;

		if (_lastSendPos + nSendLen <= SEND_BUFF_SZIE)
		{
			//拷贝数据
			memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
			//计算剩余数据位置
			_lastSendPos += nSendLen;
			if (_lastSendPos == SEND_BUFF_SZIE) {
				_sendBuffFullCount++;
			}
			return nSendLen;
		}
		else {
			// 表示缓冲区存满一次
			_sendBuffFullCount++;
		}
		return ret;
	}

	// 重置心跳计数
	void resetDTHeart()
	{
		_dtHeart = 0;
	}

	// 重置心跳计数
	void resetDTSend()
	{
		_dtSend = 0;
	}

	//检测心跳是否超时
	bool checkHeart(time_t dt)
	{
		_dtHeart += dt;
		// 超时死亡
		if (_dtHeart >= CLIENT_HREAT_DEAD_TIME)
		{
			printf("checkHeart dead:s=%d,time=%d\n",_sockfd, _dtHeart);
			return true;
		}
		return false;
	}

	void SendDataReal(netmsg_DataHeader * header) {
		SendData(header);
		SendDataReal();
	}

	// 立即发送数据，不管缓冲区是否装满
	int SendDataReal() {
		int ret = 0;
		// 缓冲区有数据
		if (_lastSendPos > 0 && SOCKET_ERROR != _sockfd ) {
			ret = send(_sockfd, _szSendBuf, _lastSendPos, 0);
			_lastSendPos = 0;
			_sendBuffFullCount = 0;
			resetDTSend();  // 发送定时清零
		}
		return ret;
	}

	//定时发送消息
	bool checkSend(time_t dt)
	{
		_dtSend += dt;
		// 超时死亡
		if (_dtSend >= CLIENT_SEND_BUFF_TIME)
		{
		// 	printf("checkSend:s=%d,time=%d\n", _sockfd, _dtSend);
			// 时间到了，立即发送数据
			SendDataReal();
			// 重置定时初值
			resetDTSend();
			return true;
		}
		return false;
	}


private:
	// socket fd_set  file desc set
	SOCKET _sockfd;
	//第二缓冲区 消息缓冲区
	char _szMsgBuf[RECV_BUFF_SZIE];
	//消息缓冲区的数据尾部位置
	int _lastPos;

	//第二缓冲区 发送缓冲区
	char _szSendBuf[SEND_BUFF_SZIE];
	//发送缓冲区的数据尾部位置
	int _lastSendPos;
	//心跳死亡计时
	time_t _dtHeart;
	// 定时发送：上次的时间
	time_t _dtSend;
	// 发送缓冲区遇到写满的情况计数
	int _sendBuffFullCount = 0;

};

#endif // !_CellClient_hpp_



