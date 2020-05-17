#ifndef _CELL_MSG_STREAM_
#define _CELL_MSG_STREAM_

#include "CELLStream.hpp"
#include "MessageHeader.hpp"
/*  --------------- 字节流协议 -------------------------
	CELLStream 是最底层的字节流类，全部基于c的数据类型展开的。
	CELLReadStream 继承CELLStream，封装一个方法：getNetCmd
	CELLWriteStream 继承CELLStream，封装两个方法：setNetCmd 、WriteString
	字节流的首个 uint16_t 存放了字节流中数据的数量
*/

class CELLReadStream :public CELLStream {
public:
	// 直接将 netmsg_DataHeader 转为字节流
	CELLReadStream(netmsg_DataHeader * header)
		// 这个构造函数，又调用下面的构造函数
		:CELLReadStream((char*)header, header->dataLength) {

	}
	// 使用pData进行初始化一个流对象
	CELLReadStream(char * pData, int nSize, bool bDelete = false)
		:CELLStream(pData, nSize, bDelete) {
		// 写入数据需要更新写位置
		push(nSize);
	}

	//
	uint16_t getNetCmd() {
		// 初始化
		uint16_t cmd = CMD_ERROR;
		// 读取字节流缓冲区首个未读的数据
		Read(cmd);
		// 
		return cmd;
	}
};

class CELLWriteStream :public CELLStream {
public:
	// 向 pData 中写入数据
	CELLWriteStream(char * pData, int nSize, bool bDelete = false)
		:CELLStream(pData, nSize, bDelete) {
		// 模板函数可以声明模板，或者根据传参自动判断类型
		Write<uint16_t>(0);
	}

	CELLWriteStream(int nSize = 1024) :CELLStream(nSize) {
		Write<uint16_t>(0);
	}

	void setNetCmd(uint16_t cmd) {
		Write(cmd);
	}

	bool WriteString(const char * str, int len) {
		return WriteArray(str, len);
	}

	bool WriteString(const char * str) {
		return WriteArray(str, strlen(str));
	}

	bool WriteString(std::string & str) {
		return WriteArray(str.c_str(), str.length());
	}
	
	// 为整个字节流缓冲区的首个 uint16_t 上写入长度
	void finish() {
		// 获取 _nWritePos 的位置（即缓冲区内元素数量）
		int pos = length();
		// 写位置清零
		setWritePos(0);
		// 第一个位置记录缓冲区的内存放的数据数量
		Write<uint16_t>(pos);
		// 复原 写位置 
		setWritePos(pos);
	}
};

#endif // ! _CELL_MSG_STREAM_