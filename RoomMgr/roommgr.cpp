#include "roommgr.h"
#include "../Dispatcher/dispatcher.h"
#include "log.hpp"
#include <boost/bind.hpp>

USING_NAMESPACE
namespace gp = google::protobuf;

RoomMgr::RoomMgr() : max_game_(10)
{
	
}

void RoomMgr::init(size_t max_game)
{
	if (max_game != 0)
		max_game_ = max_game;
	Dispatcher *dispatcher = Dispatcher::instance();
	dispatcher->Register(boost::bind(&RoomMgr::handle_request, this, _1, _2, _3));
}

void RoomMgr::handle_request(UNOMsg *msg, SessionPtr session, Timestamp ts)
{
	if (msg->type() != UNOMsg::Room)
		return;
	RoomMsg rmsg = msg->room_msg();
	switch (rmsg.type())
	{
	case RoomMsg::GetList:
		AddTask(boost::bind(&RoomMgr::GetList, this, session, msg->sequence()));
		break;
	case RoomMsg::Login:
		if (!rmsg.has_login_info())
		{
			Dispatcher::Error("login msg not completion.",session, msg->sequence());
			return;
		}
		AddTask(boost::bind(&RoomMgr::Login, this, session, msg->sequence(), rmsg.login_info()));
		break;
	case RoomMsg::Create:
		if (!rmsg.has_login_info())
		{
			Dispatcher::Error("create msg not completion.",session, msg->sequence());
			return;
		}
		AddTask(boost::bind(&RoomMgr::Create, this, session,  msg->sequence(), rmsg.login_info()));
		break;
	case RoomMsg::Setting:
		if (!rmsg.has_room_detail())
		{
			Dispatcher::Error("setting msg not completion.",session, msg->sequence());
			return;
		}
		AddTask(boost::bind(&RoomMgr::Setting, this, session, msg->sequence(), rmsg.room_detail()));
		break;
	default:
		break;
	}
}

void RoomMgr::GetList(SessionPtr session, int64 seq)
{
	LOG(debug) <<  __FUNCTION__ << " called.";
	RoomMsg *msg = new RoomMsg();
	RoomMap::iterator iter;
	GameMgr *game = NULL;
	RoomInfo *room = NULL;
	readlock lock(room_mutex_);
	for (iter = room_map_.begin(); iter != room_map_.end(); ++iter)
	{
		game = iter->second;
		if (game->isRunning())
			continue;
		room = msg->add_room_info();
		room->set_name(game->name());
		room->set_number(game->number());
		room->set_locked(game->isLocked());
	}
	lock.unlock();
	
	msg->set_type(RoomMsg::ListResult);
	std::string data;
	if (Encode(msg, data, seq))
		session->send(data);
}

void RoomMgr::Login(SessionPtr session, int64 seq, const LoginInfo &info)
{
	LOG(debug) <<  __FUNCTION__ << " called.";
	RoomMsg *msg = new RoomMsg;
	msg->set_type(RoomMsg::LoginResult);
	std::string data;
	readlock lock(room_mutex_);
	RoomMap::iterator iter = room_map_.find(info.room_number());
	int32 id = 0;
	if (iter != room_map_.end())
	{
		std::stringstream ss;
		ss << info.user() << " login to " << info.room_number();
		id = iter->second->Login(info.user(), info.password(), session);
		if (id != 0) // success
		{
			LOG(info) << ss.str() << " successfully, id: " << id;
			LoginResult *result = new LoginResult;
			result->set_myid(id);
			StatusInfo *info = new StatusInfo;
			info->set_type(StatusInfo::Room);
			info->set_allocated_room_detail(iter->second->GetRoomDetail());
			result->set_allocated_status_info(info);
			msg->set_allocated_login_result(result);
		}
		else
		{
			LOG(info) << ss.str() << " failed.";
		}
	}

	if (Encode(msg, data, seq))
		session->send(data);
	if (id != 0)
		iter->second->BroadcastAllUserInfo();
}

void RoomMgr::Create(SessionPtr session, int64 seq,const LoginInfo &info)
{
	LOG(debug) <<  __FUNCTION__ << " called.";
	readlock lock(room_mutex_);
	if (room_map_.size() >= max_game_)
	{
		Dispatcher::Error("games count has reached its maximum, can not create anymore.",session, seq);
		return;
	}

	int32 id = room_map_.size() + 1;
	GameMgr *game = new GameMgr(room_map_.size() + 1);
	if (game)
	{
		upgradelock uplock(lock);
		room_map_[id] = game;
	}
	lock.unlock();
	LOG(info) << "create a room, No." << id;
	LoginInfo info_new = info;
	info_new.set_room_number(id);
	Login(session, seq, info_new);
}

void RoomMgr::Setting(SessionPtr session, int64 seq, const RoomDetail &detail)
{
	LOG(debug) <<  __FUNCTION__ << " called.";
	if (detail.has_state())
	{
		Dispatcher::Error("you can not control the game state.", session, seq);
		return;
	}

	if (detail.has_number())
	{
		RoomMap::iterator iter;
		readlock lock(room_mutex_);
		iter = room_map_.find(detail.number());
		if (iter == room_map_.end())
		{
			Dispatcher::Error("setting, not such room.", session, seq);
			return;
		}

		if (detail.has_name())
			iter->second->SetRoomName(detail.name());
		if (detail.has_password())
			iter->second->SetRoomPassword(detail.password());
		if (detail.has_timeout())
			iter->second->SetRoomTimeOut(detail.timeout());

		iter->second->Broadcast(boost::bind(&GameMgr::SendRoomDetail, iter->second));
	}
}