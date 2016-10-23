#ifndef _UNO_QUEUE_H__
#define _UNO_QUEUE_H__
#include "defines.h"
#include "../Common/common.h"
#include "../Network/session.h"
#include "semaphore.hpp"
#include <list>

NAMESPACE_PROLOG

class Logic;
class Dispatcher;
class ChatMgr;
class Queue
{
public:
	explicit Queue(Logic *logic, ChatMgr *chat);
	virtual ~Queue();
	bool addSession(SessionPtr session);
	void onMessage(UNOMsg *msg, SessionPtr session,Timestamp timestamp);
private:
	struct Req
	{
		GameMsg msg;
		Timestamp ts;
		SessionPtr sp;
		int64 seq;
		Req(GameMsg m,SessionPtr ssp,Timestamp t,int64 s)
		{
			msg = m;
			ts = t;
			sp = ssp;
			seq = s;
		}
	};
	void ProcessMsg();
	typedef std::list<Req> GameQueue;
	GameQueue game_queue_;
	boost::shared_mutex queue_mutex_;
	Semaphore queue_active_;
	bool disposed_;
	boost::shared_ptr<boost::thread> thread_;
	Logic *logic_;
	ChatMgr *chat_;
	Dispatcher *dispatcher_;
};


NAMESPACE_EPILOG
	
#endif