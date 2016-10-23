#include "defines.h"
#include "../Common/common.h"
#include "../Network/network.h"
#include "log.hpp"
#include "semaphore.hpp"
#include <string>
#include <Shlwapi.h>

USING_NAMESPACE

void SendRequest(RoomMsg *rmsg)
{
	UNOMsg msg;
	msg.set_type(UNOMsg::Room);
	msg.set_allocated_room_msg(rmsg);
	int64 seq = generateSequence();
	LOG(debug) << __FUNCTION__ << " seq: " << seq;
	msg.set_sequence(seq);
	std::string data;
	if (Encode(msg, data))
		Network::Send(data);
}

void GetGameRoomList(std::string s)
{
	RoomMsg *rmsg = new RoomMsg;
	rmsg->set_type(RoomMsg::GetList);
	SendRequest(rmsg);
}

void CreateRoom(std::string s)
{
	RoomMsg *rmsg = new RoomMsg;
	rmsg->set_type(RoomMsg::Create);
	LoginInfo *info = new LoginInfo;
	info->set_user(generateName(8));
	info->set_room_number(0);
	rmsg->set_allocated_login_info(info);
	SendRequest(rmsg);
}

void Login(std::string room_id)
{
	if (room_id.length() == 0)
	{
		LOG(error) << "login need a room id.";
		return;
	}
	
	int rid;
	if (!StrToIntEx(room_id.c_str(),STIF_DEFAULT, &rid))
	{
		LOG(error) << "room id invalid.";
		return;
	}

	RoomMsg *rmsg = new RoomMsg;
	rmsg->set_type(RoomMsg::Login);
	LoginInfo *info = new LoginInfo;
	info->set_user(generateName(8));
	info->set_room_number(rid);
	rmsg->set_allocated_login_info(info);
	SendRequest(rmsg);
}