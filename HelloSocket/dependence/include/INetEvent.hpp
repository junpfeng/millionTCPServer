#ifndef _I_NET_EVENT_HPP_
#define _I_NET_EVENT_HPP_

#include"CELL.hpp"
#include"CELLClient.hpp"
//自定义
class CellServer;

//锟斤拷锟斤拷锟铰硷拷锟接匡拷
class INetEvent
{
public:
	//锟斤拷锟介函锟斤拷
	//锟酵伙拷锟剿硷拷锟斤拷锟铰硷拷
	virtual void OnNetJoin(CellClient* pClient) = 0;
	//锟酵伙拷锟斤拷锟诫开锟铰硷拷
	virtual void OnNetLeave(CellClient* pClient) = 0;
	//锟酵伙拷锟斤拷锟斤拷息锟铰硷拷
	virtual void OnNetMsg(CellServer* pCellServer, CellClient* pClient, netmsg_DataHeader* header) = 0;
	//recv锟铰硷拷
	virtual void OnNetRecv(CellClient* pClient) = 0;
private:

};

#endif // !_I_NET_EVENT_HPP_
