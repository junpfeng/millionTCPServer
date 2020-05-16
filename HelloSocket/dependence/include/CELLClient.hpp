#ifndef _CellClient_hpp_
#define _CellClient_hpp_

#include"CELL.hpp"
#include"CELLBuffer.hpp"

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
	CellClient(SOCKET sockfd = INVALID_SOCKET):
		_sendBuff(SEND_BUFF_SIZE),
		_recvBuff(RECV_BUFF_SIZE)
	{
		static int n = 1;
		id = n++;

		_sockfd = sockfd;

		resetDTHeart();
		resetDTSend();
	}
	~CellClient() {
		CELLLog::Info("~CellClient id = %d, serverID = %d\n", id, serverId);
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

	// 从套接字中读取数据，存入缓冲区
	int RecvData() {
		return _recvBuff.read4socket(_sockfd);
	}

	// 判断缓冲区中是否有数据
	bool hasMsg() {
		return _recvBuff.hasMsg();
	}

	// 返回缓冲区首地址（强转为netmsg_DataHeader)
	netmsg_DataHeader* front_msg() {
		return (netmsg_DataHeader*)_recvBuff.data();
	}

	// 将缓冲区的首个头数据弹出（丢弃）
	void pop_front_msg() {
		if (hasMsg()) {
			_recvBuff.pop(front_msg()->dataLength);
		}
	}

	//发送数据
	int SendData(netmsg_DataHeader* header)
	{
		return SendData((const char *)header, header->dataLength);
	}
	//发送数据
	int SendData(const char * pData, int len)
	{
		// 将一个头包数据加入缓冲区
		if (_sendBuff.push(pData, len)) {
			return len;
		}
		return SOCKET_ERROR;
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
			CELLLog::Info("checkHeart dead:s=%d,time=%d\n",_sockfd, _dtHeart);
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
		// 重载定时发送事件
		resetDTSend();
		return _sendBuff.write2socket(_sockfd);
	}

	//定时发送消息
	bool checkSend(time_t dt)
	{
		_dtSend += dt;
		// 超时死亡
		if (_dtSend >= CLIENT_SEND_BUFF_TIME)
		{
		// 	CELLLog::Info("checkSend:s=%d,time=%d\n", _sockfd, _dtSend);
			// 时间到了，立即发送数据
			SendDataReal();
			// 重置定时初值
			resetDTSend();
			return true;
		}
		return false;
	}

	bool needWrite() {
		return _sendBuff.needWrite();
	}

private:
	// socket fd_set  file desc set
	SOCKET _sockfd;
	// 收发缓冲区对象
	CELLBuffer _recvBuff;
	CELLBuffer _sendBuff;

	//心跳死亡计时
	time_t _dtHeart;
	// 定时发送：上次的时间
	time_t _dtSend;
	// 发送缓冲区遇到写满的情况计数
	int _sendBuffFullCount = 0;

};

#endif // !_CellClient_hpp_



