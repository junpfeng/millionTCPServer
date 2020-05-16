#ifndef _CELL_MSG_STREAM_
#define _CELL_MSG_STREAM_

#include "CELLStream.hpp"
#include "MessageHeader.hpp"
/*  --------------- �ֽ���Э�� -------------------------
	CELLStream ����ײ���ֽ����࣬ȫ������c����������չ���ġ�
	CELLReadStream �̳�CELLStream����װһ��������getNetCmd
	CELLWriteStream �̳�CELLStream����װ����������setNetCmd ��WriteString
	�ֽ������׸� uint16_t ������ֽ��������ݵ�����
*/

class CELLReadStream :public CELLStream {
public:
	// ֱ�ӽ� netmsg_DataHeader תΪ�ֽ���
	CELLReadStream(netmsg_DataHeader * header)
		// ������캯�����ֵ�������Ĺ��캯��
		:CELLReadStream((char*)header, header->dataLength) {

	}
	// ʹ��pData���г�ʼ��һ��������
	CELLReadStream(char * pData, int nSize, bool bDelete = false)
		:CELLStream(pData, nSize, bDelete) {
		// д��������Ҫ����дλ��
		push(nSize);
	}

	//
	uint16_t getNetCmd() {
		// ��ʼ��
		uint16_t cmd = CMD_ERROR;
		// ��ȡ�ֽ����������׸�δ��������
		Read(cmd);
		// 
		return cmd;
	}
};

class CELLWriteStream :public CELLStream {
public:
	// �� pData ��д������
	CELLWriteStream(char * pData, int nSize, bool bDelete = false)
		:CELLStream(pData, nSize, bDelete) {
		// ģ�庯����������ģ�壬���߸��ݴ����Զ��ж�����
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
	
	// Ϊ�����ֽ������������׸� uint16_t ��д�볤��
	void finish() {
		// ��ȡ _nWritePos ��λ�ã�����������Ԫ��������
		int pos = length();
		// дλ������
		setWritePos(0);
		// ��һ��λ�ü�¼���������ڴ�ŵ���������
		Write<uint16_t>(pos);
		// ��ԭ дλ�� 
		setWritePos(pos);
	}
};

#endif // ! _CELL_MSG_STREAM_