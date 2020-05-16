#ifndef _CELL_STREAM_HPP_
#define _CELL_STREAM_HPP_

#include"CELLLog.hpp"
#include<cstdint>

// 将数据的读写封装为字节流
// 在c/c++中，字节流的存储即 char 类型。
////// 字节流协议：
// 分两种：读/写单个元素；读/写数组（字符串）
// 读/写单个元素时：
// 读/写数组（字符串）时：第一个元素是表示这个字符串的长度
class CELLStream
{
public:
	// 构造：将pData地址指向_pBuff
	CELLStream(char *pData, int nSize, bool bDelete = false) {
		_nSize = nSize;
		_pBuff = pData;
		// 由于是外部申请的，默认因此由外部删除
		_bDelete = bDelete;
	}

	// 构造：指定字节流缓冲区的大小
	CELLStream(int nSize = 1024) {
		_nSize = nSize;
		_pBuff = new char[_nSize];
		// 类内申请，类内释放
		_bDelete = true;
	}

	virtual ~CELLStream() {
		if (_bDelete && _pBuff) {
			delete[] _pBuff;
			_pBuff = nullptr;
		}
	}

public:
//// 提供外部读取私有熟悉
	char * data() {
		return _pBuff;
	}

	// 返回字节流缓冲区内存放的元素数量
	int length() {
		return _nWritePos;
	}
	// 可读
	inline bool canRead(int n) {
		return _nSize - _nReadPos >= n;
	}
	// 可写
	inline bool canWrite(int n) {
		return _nSize - _nWritePos >= n;
	}

//// push就是写入数据，那么_nWritePos 就需要后移
	// 每写入n个数据，则写起始位置后移n
	inline void push(int n) {
		_nWritePos += n;
	}
//// pop就是读出数据，_nReadPos 对应后移
	// 每读入n个数据，则读起始位置后移n
	inline void pop(int n) {
		_nReadPos += n;
	}
	// 指定写入的位置
	inline void setWritePos(int n) {
		_nWritePos = n;
	}

///////Read
	// 从字节流缓冲区读取一个 T 类型的数据
	// 当 bOffset = false 之后，就是不调整读的位置（这样子就可以重读读取相同位置）
	template<typename T>  // 引用类型
	bool Read(T &  n, bool bOffset = true) {
		// 读取的基本原理，就是从将T中的数据复制到_pBuf中来
		auto nLen = sizeof(T);

		if (canRead(nLen)) {
			// 将读取的数据存到_pBuff 上第_nReadPos后面
			memcpy(&n, _pBuff + _nReadPos, nLen);
			// 默认会更新_nReadPos
			if (bOffset) {
				// 更新 _nReadPos 的位置为 _nReadPos += nLen
				pop(nLen);
				return;
			} 
			// 
			CELLLog::Info("error, CELLStream::Read failed.");
			return true;
		}
		CELLLog::Info("error, CELLStream::Read failed.");
		return false;
	}

	// 只读，不改变读起始位置
	template<typename T>
	bool onlyRead(T & n) {
		return Read(n, false);
	}

	// 读取 len 个 T 类型的数据存入到 pArr 中
	template<typename T>
	uint32_t ReadArray(T * pArr, uint32_t len) {
		uint32_t len1 = 0;
		// 读取第1个uint32_t类型的数据，但是不调整_nReadpos
		// 由于是引用传参，len1为读取的第一个数据的值，这个是和WriteArray匹配的
		// 由于WriteArray写入的第一个元素是数组的长度，因此读取时，先读第一个表示数组的长度
		// 
		Read(len1, false);
		
		if (len1 < len) {
			// 总长度
			auto nLen = len1 * sizeof(T);
			// 如果可读
			if (canRead(nLen + sizeof(uint32_t))) {
				// 调整读的起始位置
				pop(sizeof(uint32_t));
				// 拷贝
				memcpy(pArr, _pBuff + _nReadPos, nLen);
				// 调整读的起始位置
				pop(nLen);
				return len1;
			}
		}
		CELLLog::Info("error, CELLStream::ReadArray failed. ");
		return 0;
	}

	////char size_t c# char2 char 1 
	//int8_t ReadInt8(int8_t def = 0)
	//{
	//	Read(def);
	//	return def;
	//}
	////short
	//int16_t ReadInt16(int16_t n = 0)
	//{
	//	Read(n);
	//	return n;
	//}
	////int
	//int32_t ReadInt32(int32_t n = 0)
	//{
	//	Read(n);
	//	return n;
	//}
	//
	//int64_t ReadInt64(int64_t n = 0)
	//{
	//	Read(n);
	//	return n;
	//}
	//
	//uint8_t ReadUInt8(uint8_t def = 0)
	//{
	//	Read(def);
	//	return def;
	//}
	////short
	//uint16_t ReadUInt16(uint16_t n = 0)
	//{
	//	Read(n);
	//	return n;
	//}
	////int
	//uint32_t ReadUInt32(uint32_t n = 0)
	//{
	//	Read(n);
	//	return n;
	//}
	//
	//uint64_t ReadUInt64(uint64_t n = 0)
	//{
	//	Read(n);
	//	return n;
	//}
	//
	//float ReadFloat(float n = 0.0f)
	//{
	//	Read(n);
	//	return n;
	//}
	//double ReadDouble(double n = 0.0f)
	//{
	//	Read(n);
	//	return n;
	//}

//// Write
	template<typename T>
	bool Write(T n) {
		// 写入类型的大小
		auto nLen = sizeof(T);
		// 是否写的下
		if (canWrite(nLen)) {
			// 写入
			memcpy(_pBuff + _nWritePos, &n, nLen);
			// 调整写入位置
			push(nLen);
			return true;
		}
		CELLLog::Info("error, CELLStream::Write failed");
		return false;
	}

	template<typename T>
	bool WriteArray(T * pData, uint32_t len) {
		// 计算写入数组的类型T的字节长度
		auto nLen = sizeof(T)*len;
		// 判断是否能写入
		if (canWrite(nLen + sizeof(uint32_t))) {
			// 写入数组的元素数量
			Write(len);
			memcpy(_pBuff + _nWritePos, pData, nLen);
			// 将写入的数据，拷贝到缓冲区尾部
			push(nLen);
			return true;
		}
		CELLLog::Info("error, CELLStream::WriteArray failed.");
		return false;
	}

////// Write
//	bool WriteInt8(int8_t n)
//	{
//		return Write(n);
//	}
//	//short
//	bool WriteInt16(int16_t n)
//	{
//		return Write(n);
//	}
//
//	//int
//	bool WriteInt32(int32_t n)
//	{
//		return Write(n);
//	}
//
//	bool WriteFloat(float n)
//	{
//		return Write(n);
//	}
//
//	bool WriteDouble(double n)
//	{
//		return Write(n);
//	}
private:
	// 字节流数据缓冲区，在c/c++中，使用 char 来进行字节流的保存
	// 这一点其实要联系到在 intel 实习中的项目中的，Python 到c/c++之间
	// 传输参数等数据时，就是基于字节流 char 传输
	char * _pBuff = nullptr;
	// 缓冲区的总大小
	int _nSize = 0;
//// _nWritePos 和 _nReadPos 共同指向一块内存
	// 记录写入数据的位置
	int _nWritePos = 0;
	// 记录已读数据的位置
	int _nReadPos = 0;
	// 类似于智能指针的引用计数，只不过这个只有可删和不可删两种
	bool _bDelete = true;
};

#endif // _CELL_STREAM_HPP_