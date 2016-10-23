#include "usermgr.h"
#include "gamemgr.h"
#include "log.hpp"

USING_NAMESPACE

UserMgr::UserMgr() : max_id_(1)
{

}

UserMgr::~UserMgr()
{

}

int32 UserMgr::Add( const std::string &name)
{
	int32 id = max_id_++;
	UserInfo user;
	user.set_name(name);
	user.set_id(id);
	user.set_state(UserInfo::idle);
	user.set_card_count(0);
	writelock lock(user_mutex_);
	user_map_[id] = user;
	return id;
}

void UNO::UserMgr::Remove(int32 id)
{
	UserMap::iterator iter;
	readlock lock(user_mutex_);
	iter = user_map_.find(id);
	if (iter != user_map_.end())
	{
		upgradelock uplock(lock);
		user_map_.erase(iter);
	}
}

bool UserMgr::GetUser(int32 id, UserInfo *user)
{
	readlock lock(user_mutex_);
	UserMap::iterator iter = user_map_.find(id);
	if (iter != user_map_.end())
	{
		if (user != NULL)		
			user->CopyFrom(iter->second);
		return true;
	}

	return false;
}

bool UserMgr::SetUser(int32 id, int count)
{
	readlock lock(user_mutex_);
	UserMap::iterator iter = user_map_.find(id);
	if (iter != user_map_.end())
	{
		upgradelock uplock(lock);
		iter->second.set_card_count(count);
		return true;
	}

	return false;
}

bool UNO::UserMgr::SetUser(int32 id, UserInfo::State state)
{
	readlock lock(user_mutex_);
	UserMap::iterator iter = user_map_.find(id);
	if (iter != user_map_.end())
	{
		upgradelock uplock(lock);
		iter->second.set_state(state);
		return true;
	}

	return false;
}

int32 UserMgr::GetUserCount()
{
	readlock lock(user_mutex_);
	return user_map_.size();
}

int32 UserMgr::GetFirstUser()
{
	readlock lock(user_mutex_);
	return user_map_.begin()->first;
}

int32 UserMgr::WhoIsNext(int32 now, bool reverse)
{
	UserMap::iterator iter;
	readlock lock(user_mutex_);
	iter = user_map_.find(now);
	if (iter != user_map_.end())
	{
		if (!reverse)
		{
			if (++iter == user_map_.end())
				iter = user_map_.begin();
		}
		else
		{
			if (iter == user_map_.begin())
				iter = user_map_.end();
			--iter;
		}
	}
	else	
	{
		// now user offline
		if (!reverse)
		{
			for (iter = user_map_.begin(); iter != user_map_.end(); ++iter)
			{
				if (iter->first > now)
					break;
			}

			if (iter == user_map_.end())
				iter = user_map_.begin();
		}
		else
		{
			iter = user_map_.end();
			do 
			{
				--iter;
				if (iter->first < now)
					break;
			} while (iter != user_map_.begin());

			if (iter == user_map_.begin())
			{
				if (iter->first > now)
				{
					iter = user_map_.end();
					--iter;
				}
			}
		}
	}

	return iter->first;
}

bool UserMgr::isEveryoneReady()
{
	UserMap::iterator iter;
	readlock lock(user_mutex_);
	for (iter = user_map_.begin(); iter != user_map_.end(); ++iter)
	{
		if (iter->second.state() != UserInfo::ready)
			return false;
	}

	return true;
}

void UserMgr::reset()
{
	UserMap::iterator iter;
	writelock lock(user_mutex_);
	for (iter = user_map_.begin(); iter != user_map_.end(); ++iter)
	{
		iter->second.set_state(UserInfo::idle);	
		iter->second.set_card_count(0);
	}
}

std::string UserMgr::NotifyUser(int32 update_id)
{
	// call whenever user info updated
	GameMsg *gmsg = new GameMsg;
	gmsg->set_type(GameMsg::Status);
	StatusInfo *status = new StatusInfo;
	status->set_type(StatusInfo::User);
	gmsg->set_allocated_status_info(status);
	UserMap::iterator iter;
	if (update_id == 0) // all user
	{
		readlock lock(user_mutex_);
		UserInfo *user;
		status->set_all_user_updated(true);
		for (iter = user_map_.begin(); iter != user_map_.end(); ++iter)
		{
			user = status->add_user_info();
			user->CopyFrom(iter->second);
		}
	}
	else
	{
		readlock lock(user_mutex_);
		iter = user_map_.find(update_id);
		status->set_all_user_updated(false);
		if (iter != user_map_.end())
		{
			UserInfo *user = status->add_user_info();
			user->CopyFrom(iter->second);
		}
		else
		{
			LOG(warning) << __FUNCTION__ << " user " << update_id << " not found";
		}
	}

	std::string data = "";
	if (!Encode(gmsg, data, 0))
	{
		LOG(warning) << __FUNCTION__ << " encode failed.";
	}

	return data;
}

std::list<int32> UserMgr::GetUserList()
{
	readlock lock(user_mutex_);
	std::list<int32> result_list;
	UserMap::iterator iter;
	for (iter = user_map_.begin(); iter != user_map_.end(); ++iter)
	{
		result_list.push_back(iter->first);
	}

	return result_list;
}
