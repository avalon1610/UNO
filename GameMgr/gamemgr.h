#ifndef _UNO_GAMEMGR_H__
#define _UNO_GAMEMGR_H__
#include "defines.h"
#include "../Common/common.h"
#include "../Network/session.h"
#include "../Network/timer.h"
#include <boost/bimap.hpp>
#include <map>
NAMESPACE_PROLOG

class UserMgr;
class CardMgr;
class ChatMgr;
class Queue;
class Logic;
typedef boost::function<std::string ()> MsgAssembler;
class GameMgr
{
public:
	explicit GameMgr(int32 id, std::string name = "A UNO game room");
	virtual ~GameMgr();
	std::string name() const { return name_; }
	int32 number() const { return number_; }
	void Broadcast(MsgAssembler);
	bool SendToUser(int32, MsgAssembler);
	void SystemBroadcast(int, std::string content);
	int32 Login(std::string user, std::string password, SessionPtr session);
	int32 GetUserID(SessionPtr session);
	RoomDetail * GetRoomDetail();
	std::string SendRoomDetail();
	void SetRoomTimeOut(int32 timeout) { timeout_ = timeout; }
	void SetRoomPassword(std::string password) { password_ = password; }
	void SetRoomName(std::string name) { name_ = name; }
	void Start();
	void Stop();
	bool isRunning() { return running_; }
	bool isLocked() { return (password_.length() != 0); }
	void ResetTimer();
	void BroadcastAllUserInfo();
private:
	void onTimeout();
	void onSessionClose(SessionPtr session);
	typedef boost::bimap<int32, SessionPtr> UserMap;
	UserMap user_map_;
	boost::shared_mutex user_mutex_;
	std::string password_;
	std::string name_;
	int32 number_;
	UserMgr *usermgr;
	CardMgr *cardmgr;
	ChatMgr *chatmgr;
	bool running_;
	int32 timeout_;
	Queue *queue_;
	Logic *logic_;
	typedef boost::function<void ()> TimeoutCallback;
	Timer<TimeoutCallback> *timer_;
};

NAMESPACE_EPILOG
#endif