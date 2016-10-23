#include "chatmgr.h"
#include "gamemgr.h"
#include "log.hpp"

USING_NAMESPACE

ChatMgr::ChatMgr(GameMgr *game) : game_(NULL)
{
	game_ = game;
}

void ChatMgr::handle_message(const ChatMsg &msg, SessionPtr session, Timestamp timestamp, int64 seq)
{
	if (game_ == NULL)
		return;
	
	int32 id = game_->GetUserID(session);
	if (id == 0)
		return;

	game_->Broadcast(boost::bind(&ChatMgr::SendMessage, this, id, msg.content(), timestamp));
}

std::string ChatMgr::SendMessage(int32 id, std::string content, Timestamp timestamp)
{
	ChatMsg *new_msg = new ChatMsg;
	new_msg->set_time(timestamp);
	new_msg->set_content(content);
	new_msg->set_userid(id);
	std::string data;
	if (!Encode(new_msg, data, 0))
		LOG(error) << __FUNCTION__ << " encode failed.";
	return data;
}