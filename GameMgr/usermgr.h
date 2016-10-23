#ifndef _UNO_USER_MGR_H__
#define _UNO_USER_MGR_H__
#include "defines.h"
#include "../Common/common.h"
#include <map>
#include <string>
#include <list>

NAMESPACE_PROLOG

class UserMgr
{
public:
	UserMgr();
	virtual ~UserMgr();

	int32 Add(const std::string &name);
	void Remove(int32 id);
	bool GetUser(int32 id, UserInfo *user = NULL);
	bool SetUser(int32 id, UserInfo::State state);
	bool SetUser(int32 id, int32 count);
	int32 GetUserCount();
	int32 GetFirstUser();
	int32 WhoIsNext(int32 now, bool reverse);
	bool isEveryoneReady();
	void reset();
	std::string NotifyUser(int32 update_id = 0);
	std::list<int32> GetUserList();
private:
	typedef std::map<int32, UserInfo> UserMap;
	UserMap user_map_;
	boost::shared_mutex user_mutex_;
	int32 max_id_;
};

NAMESPACE_EPILOG
#endif