#ifndef __UNO_ROOM_MGR_H__
#define __UNO_ROOM_MGR_H__
#include "defines.h"
#include "singleton.hpp"
#include "../Common/common.h"
#include "../GameMgr/gamemgr.h"
#include "../Network/session.h"
#include <boost/thread/shared_mutex.hpp>
#include <map>

NAMESPACE_PROLOG

SINGLETON_CLASS_BEGIN(RoomMgr)
public:
	virtual ~RoomMgr() {}
	void init(size_t max_game = 0);
private:
	RoomMgr();
	void handle_request(UNOMsg *msg, SessionPtr, Timestamp);
	void GetList(SessionPtr, int64 seq);
	void Login(SessionPtr, int64 seq, const LoginInfo &info);
	void Create(SessionPtr, int64 seq, const LoginInfo &info);
	void Setting(SessionPtr, int64 seq, const RoomDetail &detail);
	typedef std::map<int32, GameMgr *> RoomMap;
	RoomMap room_map_;
	boost::shared_mutex room_mutex_;
	size_t max_game_;
SINGLETON_CLASS_END

NAMESPACE_EPILOG
#endif