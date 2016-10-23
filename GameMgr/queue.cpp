#include "queue.h"
#include "../Dispatcher/dispatcher.h"
#include "logic.h"
#include "log.hpp"
#include "chatmgr.h"

USING_NAMESPACE
Queue::Queue(Logic *logic,ChatMgr *chat) : disposed_(false), dispatcher_(NULL),
	logic_(NULL), chat_(NULL), queue_active_(0)
{
	dispatcher_ = Dispatcher::instance();
	thread_.reset(new boost::thread(boost::bind(&Queue::ProcessMsg, this)));
	logic_ = logic;
	chat_ = chat;
}

Queue::~Queue()
{
	disposed_ = true;
	queue_active_.signal();
	thread_->join();
	if (logic_)
	{
		delete logic_;
		logic_ = NULL;
	}
	if (chat_)
	{
		delete chat_;
		chat_ = NULL;
	}
}

bool Queue::addSession(SessionPtr session)
{
	if (dispatcher_)
	{
		if (!dispatcher_->Register(boost::bind(&Queue::onMessage, this, _1, _2, _3), session))
		{
			Dispatcher::Error("You have already login.", session);
			return false;
		}

		return true;
	}

	LOG(fatal) << "dispather instance is NULL !";
	return false;
}

void Queue::onMessage(UNOMsg *msg, SessionPtr session,Timestamp timestamp)
{
	if ( msg->type() != UNOMsg::Game && msg->type() != UNOMsg::Chat)
	{
		LOG(warning) << "queue get error message.";
		return;
	}

	if (msg->type() == UNOMsg::Game)
	{
		Req r(msg->game_msg(), session, timestamp, msg->sequence());
		writelock lock(queue_mutex_);
		game_queue_.push_back(r);
		queue_active_.signal();
	}
	else if (msg->type() == UNOMsg::Chat)
	{
		chat_->handle_message(msg->chat_msg(),session,timestamp,msg->sequence());
	}
}

void Queue::ProcessMsg()
{
	GameQueue::iterator iter;
	while (true)
	{
		queue_active_.wait();
		if (disposed_)
			break;
		readlock lock(queue_mutex_);
		if (game_queue_.empty())
			continue;
		
		iter = game_queue_.begin();
		Timestamp ts;
		while (true)
		{
			ts = iter->ts;
			if (++iter == game_queue_.end())
				break;
			if (ts <= iter->ts)
				break;
		}
		--iter;

		logic_->handle_message(iter->msg, iter->sp, iter->seq);
		upgradelock uplock(lock);
		game_queue_.erase(iter);
	}
}