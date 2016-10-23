#include "defines.h"
#include "../Common/common.h"
#include "../Network/network.h"
#include "log.hpp"
#include "semaphore.hpp"
#include <string>
#include <shlwapi.h>

USING_NAMESPACE

void SendRequest(GameMsg *gmsg)
{
	UNOMsg msg;
	msg.set_type(UNOMsg::Game);
	msg.set_allocated_game_msg(gmsg);
	int64 seq = generateSequence();
	LOG(debug) << __FUNCTION__ " seq: " << seq;
	msg.set_sequence(seq);
	std::string data;
	if (Encode(msg, data))
		Network::Send(data);
}

void Ready(std::string s)
{
	GameMsg *gmsg = new GameMsg;
	gmsg->set_type(GameMsg::Ready);
	SendRequest(gmsg);
}

void DeleteCard(int index);
void PlayCard(int card_1, int card_2 = -1)
{
	GameMsg *gmsg = new GameMsg;
	gmsg->set_type(GameMsg::PlayCard);
	CardInfo *card_info = new CardInfo;
	card_info->add_number(card_1);
	DeleteCard(card_1);
	card_info->set_count(1);
	if (card_2 != -1)
	{
		card_info->add_number(card_2);
		DeleteCard(card_2);
		card_info->set_count(2);
	}
	gmsg->set_allocated_card_info(card_info);
	SendRequest(gmsg);
}

void Play(std::string s)
{
	int card;
	if (!StrToIntEx(s.c_str(), STIF_DEFAULT, &card))
	{
		LOG(error) << "play card id invalid.";
		return;
	}
	
	PlayCard(card);
}

void Play2(std::string s)
{
	int index = s.find(" ");
	if (index == std::string::npos)
	{
		LOG(error) << "play 2 card parameter invalid, need two card";
		return;
	}

	std::string temp = s.substr(0, index);
	int card_1, card_2;
	if (!StrToIntEx(temp.c_str(), STIF_DEFAULT, &card_1))
	{
		LOG(error) << "play 2 card 1 id invalid.";
		return;
	}

	temp = s.substr(index + 1);
	if (!StrToIntEx(temp.c_str(), STIF_DEFAULT, &card_2))
	{
		LOG(error) << "play 2 card 2 id invalid.";
		return;
	}

	PlayCard(card_1, card_2);
}

void Draw(std::string s)
{
	GameMsg *gmsg = new GameMsg;
	gmsg->set_type(GameMsg::DrawCard);
	SendRequest(gmsg);
}

void Done(std::string)
{
	GameMsg *gmsg = new GameMsg;
	gmsg->set_type(GameMsg::Done);
	SendRequest(gmsg);
}

void Uno(std::string)
{
	GameMsg *gmsg = new GameMsg;
	gmsg->set_type(GameMsg::UNO);
	SendRequest(gmsg);
}

void Doubt(std::string param)
{
	DoubtInfo *doubt = NULL;
	if (param.length() != 0)
	{
		doubt = new DoubtInfo;
		int id;
		if (!StrToIntEx(param.c_str(), STIF_DEFAULT, &id))
		{
			LOG(error) << "doubt id invalid.";
			return;
		}
		doubt->set_user_id(id);
	}
	GameMsg *gmsg = new GameMsg;
	gmsg->set_type(GameMsg::Doubt);
	if (doubt != NULL)
		gmsg->set_allocated_doubt_info(doubt);
	SendRequest(gmsg);
}

void Color(std::string c)
{
	GameMsg::ColorInfo color_info;
	if (c == "red")
		color_info = GameMsg::Red;
	else if (c == "green")
		color_info = GameMsg::Green;
	else if (c == "blue")
		color_info = GameMsg::Blue;
	else if (c == "yellow")
		color_info = GameMsg::Yellow;
	else
	{
		LOG(error) << __FUNCTION__ << "param invalid.";
		return;
	}

	GameMsg *gmsg = new GameMsg;
	gmsg->set_type(GameMsg::Black);
	gmsg->set_color_info(color_info)	;
	SendRequest(gmsg);
}