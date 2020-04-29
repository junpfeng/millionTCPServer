#ifndef _I_NET_EVENT_HPP_
#define _I_NET_EVENT_HPP_

#include"CELL.hpp"
#include"CELLClient.hpp"
//�Զ���
class CellServer;

//�����¼��ӿ�
class INetEvent
{
public:
	//���麯��
	//�ͻ��˼����¼�
	virtual void OnNetJoin(CellClient* pClient) = 0;
	//�ͻ����뿪�¼�
	virtual void OnNetLeave(CellClient* pClient) = 0;
	//�ͻ�����Ϣ�¼�
	virtual void OnNetMsg(CellServer* pCellServer, CellClient* pClient, netmsg_DataHeader* header) = 0;
	//recv�¼�
	virtual void OnNetRecv(CellClient* pClient) = 0;
private:

};

#endif // !_I_NET_EVENT_HPP_
