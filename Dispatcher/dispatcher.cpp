#include "dispatcher.h"
#include "../Network/network.h"
#include "log.hpp"
#include <boost/bind.hpp>

USING_NAMESPACE

Dispatcher::Dispatcher()
{
	Network::Register(boost::bind(&Dispatcher::onMessage, this, _1, _2));
	Session::subscribe(close,0,boost::bind(&Dispatcher::onSessionClose, this, _1));
}

Dispatcher::~Dispatcher()
{
	google::protobuf::ShutdownProtobufLibrary();
}

void Dispatcher::onSessionClose(SessionPtr session)
{
	LOG(debug) << "dispatcher session close";
	DispatcherMap::iterator iter;
	readlock lock(map_mutex_);
	iter = dispatch_map_.find(session);
	if (iter != dispatch_map_.end())
	{
		upgradelock uplock(lock);
		dispatch_map_.erase(iter);
	}
}

void Dispatcher::onMessage(const std::string &data,SessionPtr session)
{
	UNOMsg msg;
	if (!Decode(msg, data))
		return;

	if (!msg.has_type())
		return;

	DispatcherMap::iterator iter;
	switch (msg.type())
	{
	case UNOMsg::Room:
		if (room_handler_)
			room_handler_(&msg, session, getTimestamp());
		break;
	case UNOMsg::Game:
	case UNOMsg::Chat:
	{
		readlock lock(map_mutex_);
		if (dispatch_map_.size() == 0)
		{
			Error("No game exits now, try create one.", session);
			break;
		}
		iter = dispatch_map_.find(session);
		if (iter != dispatch_map_.end())
			iter->second(&msg, session, getTimestamp());
		else
			Error("You have not join any game.", session);
		break;
	}
	default:
		break;
	}
}


bool Dispatcher::Register(const MsgHandler &handler, SessionPtr session)
{
	if (session == NULL)
	{
		room_handler_ = handler;
		return true;
	}

	DispatcherMap::iterator iter;
	readlock lock(map_mutex_);
	iter = dispatch_map_.find(session);
	if (iter != dispatch_map_.end())
		return false;

	upgradelock uplock(lock);
	dispatch_map_[session] = handler;
	return true;
}

void Dispatcher::Error(std::string error_msg, SessionPtr session, int64 seq)
{
	LOG(warning) << error_msg;
	UNOMsg msg;
	msg.set_type(UNOMsg::Error);
	msg.set_error_msg(error_msg);
	msg.set_sequence(seq);
	std::string data;
	if (Encode(msg, data))
		session->send(data);
}