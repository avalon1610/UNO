#ifndef __UNO_CHAT_MGR_H_
#define __UNO_CHAT_MGR_H_ 
#include "defines.h"
#include "../Common/common.h"
#include "../Network/session.h"

NAMESPACE_PROLOG

class GameMgr;
class ChatMgr
{
public:
	explicit ChatMgr(GameMgr *game);
	void handle_message(const ChatMsg &msg, SessionPtr session,Timestamp timestamp, int64 seq);
	std::string ChatMgr::SendMessage(int32 id, std::string content, Timestamp timestamp);
private:
	GameMgr *game_;
};


NAMESPACE_EPILOG
#endif