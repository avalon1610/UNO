#include "gamemgr.h"
#include "usermgr.h"
#include "log.hpp"
#include "queue.h"
#include "cardmgr.h"
#include "logic.h"
#include "chatmgr.h"

USING_NAMESPACE

GameMgr::GameMgr(int32 id, std::string name) : number_(id), cardmgr(NULL), chatmgr(NULL),
	name_(name), usermgr(NULL), running_(false), timeout_(15),queue_(NULL),password_(""),logic_(NULL)
{
	usermgr = new UserMgr();
	cardmgr = new CardMgr();
	chatmgr = new ChatMgr(this);
	logic_ = new Logic(this, usermgr, cardmgr);
	queue_ = new Queue(logic_, chatmgr);
	Session::subscribe(close,0,boost::bind(&GameMgr::onSessionClose, this, _1));
	timer_ = new Timer<TimeoutCallback>(timeout_, boost::bind(&GameMgr::onTimeout, this));
}

GameMgr::~GameMgr()
{
	if (chatmgr != NULL)
	{
		delete chatmgr;
		chatmgr = NULL;
	}

	if (usermgr != NULL)
	{
		delete usermgr;
		usermgr = NULL;
	}

	if (cardmgr != NULL)
	{
		delete cardmgr;
		cardmgr = NULL;
	}

	if (queue_ != NULL)
	{
		delete queue_;
		queue_ = NULL;
	}

	if (logic_ != NULL)
	{
		delete logic_;
		logic_ = NULL;
	}
}

bool GameMgr::SendToUser(int32 id, MsgAssembler assembler)
{
	UserMap::left_map::iterator iter;
	readlock lock(user_mutex_);
	iter = user_map_.left.find(id);
	std::string data = assembler();
	if (iter != user_map_.left.end())
	{
		if (!iter->second->isAlive())
		{
			LOG(warning) << "user " << iter->first << "is offline.";
			upgradelock ulock(lock);
			user_map_.left.erase(iter++);
			return false;
		}

		iter->second->send(data);
		return true;
	}

	return false;
}

void GameMgr::SystemBroadcast(int id, std::string content)
{
	UserInfo info;
	if (!usermgr->GetUser(id, &info))
		return;
	std::string msg = "User " + info.name() + ": " + content;
	LOG(warning) << msg;
	Broadcast(boost::bind(&ChatMgr::SendMessage, chatmgr, 0, msg, getTimestamp()));
}

void GameMgr::Broadcast(MsgAssembler assembler)
{
	UserMap::left_map::iterator iter;
	readlock lock(user_mutex_);
	std::string data = assembler();
	for (iter = user_map_.left.begin(); iter != user_map_.left.end();)
	{
		if (!iter->second->isAlive())
		{
			LOG(warning) << "user " << iter->first << " is offline.";
			upgradelock ulock(lock);
			user_map_.left.erase(iter++);
			continue;
		}
		
		iter->second->send(data);
		++iter;
	}
}

int32 GameMgr::Login(std::string user, std::string password, SessionPtr session)
{
	if (password_.length() != 0 && password != password_)
	{
		LOG(debug) << "room " << number_ << " password: " << password <<" not correct.";
		return 0;
	}

	if (usermgr == NULL)
		return 0;

	if (usermgr->GetUserCount() >= 8)
		return 0;

	if (!queue_->addSession(session))
		return 0;

	int32 id = usermgr->Add(user);
	writelock lock(user_mutex_);
	user_map_.insert(UserMap::value_type(id, session));
	lock.unlock(); 
	return id;
}

void GameMgr::BroadcastAllUserInfo()
{
	Broadcast(boost::bind(&UserMgr::NotifyUser, usermgr, 0));
}

void GameMgr::Stop()
{
	running_ = false;
	cardmgr->reset();
	usermgr->reset();
	logic_->reset();
	Broadcast(boost::bind(&GameMgr::SendRoomDetail, this));
	Broadcast(boost::bind(&UserMgr::NotifyUser, usermgr, 0));
	timer_->stop();
}

void GameMgr::ResetTimer()
{
	timer_->reset(timeout_);
}

void GameMgr::Start()
{
	running_ = true; 
	Broadcast(boost::bind(&GameMgr::SendRoomDetail, this));
	timer_->start();
}

void GameMgr::onTimeout()
{
	LOG(trace) << timeout_ << "s elapsed, time out!";
	UNOMsg msg;
	msg.set_type(UNOMsg::Game);
	msg.set_sequence(0);
	GameMsg *gmsg = new GameMsg;
	gmsg->set_type(GameMsg::Timeout);
	msg.set_allocated_game_msg(gmsg);
	queue_->onMessage(&msg, NULL, getTimestamp());
}

int32 GameMgr::GetUserID(SessionPtr session)
{
	UserMap::right_map::iterator iter;
	readlock lock(user_mutex_);
	iter = user_map_.right.find(session);
	if (iter != user_map_.right.end())
	{
		return iter->second;
	}

	return 0;
}

void GameMgr::onSessionClose(SessionPtr session)
{
	UserMap::right_iterator riter;
	{
		readlock lock(user_mutex_);
		riter = user_map_.right.find(session);
		if (riter != user_map_.right.end())
		{
			usermgr->Remove(riter->second);
			upgradelock uplock(lock);
			user_map_.right.erase(riter);
		}
		else
			return;
	}

	LOG(debug) << "game manager session close";
	Broadcast(boost::bind(&UserMgr::NotifyUser, usermgr, 0));
	if (running_)
	{
		if (usermgr->GetUserCount() == 0)
		{
			Stop();
		}
		else if (usermgr->GetUserCount() == 1)
		{
			int id = usermgr->GetFirstUser();
			usermgr->SetUser(id, UserInfo::idle);
			Stop();
		}
	}

	if (usermgr->GetUserCount() == 0)
	{
		password_ = "";
	}
}

RoomDetail * GameMgr::GetRoomDetail()
{
	RoomDetail *detail = new RoomDetail;	// do not delete it, protobuf will handle it.
	detail->set_name(name_);
	detail->set_number(number_);
	detail->set_state((running_ ? RoomDetail::Running : RoomDetail::Wait));
	detail->set_timeout(timeout_);
	return detail;
}

std::string GameMgr::SendRoomDetail()
{
	GameMsg *gmsg = new GameMsg;
	gmsg->set_type(GameMsg::Status);
	StatusInfo *status = new StatusInfo;
	status->set_type(StatusInfo::Room);
	status->set_allocated_room_detail(GetRoomDetail());
	gmsg->set_allocated_status_info(status);
	std::string data;
	if (!Encode(gmsg, data, 0))
		LOG(error) << __FUNCTION__ << " encode failed.";
	return data;
}
