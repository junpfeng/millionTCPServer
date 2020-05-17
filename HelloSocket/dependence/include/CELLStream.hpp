#ifndef _CELL_STREAM_HPP_
#define _CELL_STREAM_HPP_

#include"CELLLog.hpp"
#include<cstdint>

// �����ݵĶ�д��װΪ�ֽ���
// ��c/c++�У��ֽ����Ĵ洢�� char ���͡�
////// �ֽ���Э�飺
// �����֣���/д����Ԫ�أ���/д���飨�ַ�����
// ��/д����Ԫ��ʱ��
// ��/д���飨�ַ�����ʱ����һ��Ԫ���Ǳ�ʾ����ַ����ĳ���
class CELLStream
{
public:
	// ���죺��pData��ַָ��_pBuff
	CELLStream(char *pData, int nSize, bool bDelete = false) {
		_nSize = nSize;
		_pBuff = pData;
		// �������ⲿ����ģ�Ĭ��������ⲿɾ��
		_bDelete = bDelete;
	}

	// ���죺ָ���ֽ����������Ĵ�С
	CELLStream(int nSize = 1024) {
		_nSize = nSize;
		_pBuff = new char[_nSize];
		// �������룬�����ͷ�
		_bDelete = true;
	}

	virtual ~CELLStream() {
		if (_bDelete && _pBuff) {
			delete[] _pBuff;
			_pBuff = nullptr;
		}
	}

public:
//// �ṩ�ⲿ��ȡ˽����Ϥ
	char * data() {
		return _pBuff;
	}

	// �����ֽ����������ڴ�ŵ�Ԫ������
	int length() {
		return _nWritePos;
	}
	// �ɶ�
	inline bool canRead(int n) {
		return _nSize - _nReadPos >= n;
	}
	// ��д
	inline bool canWrite(int n) {
		return _nSize - _nWritePos >= n;
	}

//// push����д�����ݣ���ô_nWritePos ����Ҫ����
	// ÿд��n�����ݣ���д��ʼλ�ú���n
	inline void push(int n) {
		_nWritePos += n;
	}
//// pop���Ƕ������ݣ�_nReadPos ��Ӧ����
	// ÿ����n�����ݣ������ʼλ�ú���n
	inline void pop(int n) {
		_nReadPos += n;
	}
	// ָ��д���λ��
	inline void setWritePos(int n) {
		_nWritePos = n;
	}

///////Read
	// ���ֽ�����������ȡһ�� T ���͵�����
	// �� bOffset = false ֮�󣬾��ǲ���������λ�ã������ӾͿ����ض���ȡ��ͬλ�ã�
	template<typename T>  // ��������
	bool Read(T &  n, bool bOffset = true) {
		// ��ȡ�Ļ���ԭ�������Ǵӽ�T�е����ݸ��Ƶ�_pBuf����
		auto nLen = sizeof(T);

		if (canRead(nLen)) {
			// ����ȡ�����ݴ浽_pBuff �ϵ�_nReadPos����
			memcpy(&n, _pBuff + _nReadPos, nLen);
			// Ĭ�ϻ����_nReadPos
			if (bOffset) {
				// ���� _nReadPos ��λ��Ϊ _nReadPos += nLen
				pop(nLen);
			} 
			return true;
		}
		CELLLog::Info("error, CELLStream::Read failed.");
		return false;
	}

	// ֻ�������ı����ʼλ��
	template<typename T>
	bool onlyRead(T & n) {
		return Read(n, false);
	}

	// ��ȡ len �� T ���͵����ݴ��뵽 pArr ��
	template<typename T>
	uint32_t ReadArray(T * pArr, uint32_t len) {
		uint32_t len1 = 0;
		// ��ȡ��1��uint32_t���͵����ݣ����ǲ�����_nReadpos
		// ���������ô��Σ�len1Ϊ��ȡ�ĵ�һ�����ݵ�ֵ������Ǻ�WriteArrayƥ���
		// ����WriteArrayд��ĵ�һ��Ԫ��������ĳ��ȣ���˶�ȡʱ���ȶ���һ����ʾ����ĳ���
		// 
		Read(len1, false);
		
		if (len1 < len) {
			// �ܳ���
			auto nLen = len1 * sizeof(T);
			// ����ɶ�
			if (canRead(nLen + sizeof(uint32_t))) {
				// ����������ʼλ��
				pop(sizeof(uint32_t));
				// ����
				memcpy(pArr, _pBuff + _nReadPos, nLen);
				// ����������ʼλ��
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
		// д�����͵Ĵ�С
		auto nLen = sizeof(T);
		// �Ƿ�д����
		if (canWrite(nLen)) {
			// д��
			memcpy(_pBuff + _nWritePos, &n, nLen);
			// ����д��λ��
			push(nLen);
			return true;
		}
		CELLLog::Info("error, CELLStream::Write failed");
		return false;
	}

	template<typename T>
	bool WriteArray(T * pData, uint32_t len) {
		// ����д�����������T���ֽڳ���
		auto nLen = sizeof(T)*len;
		// �ж��Ƿ���д��
		if (canWrite(nLen + sizeof(uint32_t))) {
			// д�������Ԫ������
			Write(len);
			memcpy(_pBuff + _nWritePos, pData, nLen);
			// ��д������ݣ�������������β��
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
	// �ֽ������ݻ���������c/c++�У�ʹ�� char �������ֽ����ı���
	// ��һ����ʵҪ��ϵ���� intel ʵϰ�е���Ŀ�еģ�Python ��c/c++֮��
	// �������������ʱ�����ǻ����ֽ��� char ����
	char * _pBuff = nullptr;
	// ���������ܴ�С
	int _nSize = 0;
//// _nWritePos �� _nReadPos ��ָͬ��һ���ڴ�
	// ��¼д�����ݵ�λ��
	int _nWritePos = 0;
	// ��¼�Ѷ����ݵ�λ��
	int _nReadPos = 0;
	// ����������ָ������ü�����ֻ�������ֻ�п�ɾ�Ͳ���ɾ����
	bool _bDelete = true;
};

#endif // _CELL_STREAM_HPP_