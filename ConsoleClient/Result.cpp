#include "../Network/session.h"
#include "../Common/common.h"
#include "log.hpp"
#include "semaphore.hpp"
#include "../Common/Cards.h"
#include <string>
#include <sstream>
#include <queue>

USING_NAMESPACE
std::list<int> now_cards;
struct ColorType
{
	Color color;
	std::string str;
};

struct FunctionalType
{
	Functional func;
	std::string str;
};

ColorType ColorCollection[] = 
{
	{Red,	"Red"},
	{Green, "Green"},
	{Blue,	"Blue"},
	{Yellow,"Yellow"},
	{Black,	"Black"}
};

FunctionalType FunctionalColllection[] =
{
	{Reverse,	"Reverse"},
	{Skip,		"Skip"},
	{Plus2,		"Plus2"},
	{Change,	"Change"},
	{Plus4,		"Plus4"}	
};

void ShowCard(int index)
{
	std::cout << "[" << index << "]";
	for (int j = 0; j < sizeof(ColorCollection) / sizeof(ColorType); ++j)
	{
		if (ColorCollection[j].color == CardCollection[index].color)
		{
			std::cout << ColorCollection[j].str;
			break;
		}
	}
	std::cout << "-";

	if (CardCollection[index].CardNumber <= 9)
		std::cout << CardCollection[index].CardNumber;
	else
	{
		for (int k = 0; k < sizeof(FunctionalColllection) / sizeof(FunctionalType); ++k)
		{
			if (FunctionalColllection[k].func == CardCollection[index].CardFunction)
			{
				std::cout << FunctionalColllection[k].str;
				break;
			}
		}
	}
}

void Show(std::string)
{
	std::list<int>::iterator iter;
	for (iter = now_cards.begin(); iter != now_cards.end(); ++iter)
	{
		ShowCard(*iter);
		std::cout << " ";
	}
	std::cout << std::endl;
}

void AddCard(int index)
{
	now_cards.push_back(index);
}

void DeleteCard(int index)
{
	std::list<int>::iterator iter;
	for (iter = now_cards.begin(); iter != now_cards.end(); ++iter)
	{
		if (*iter == index)
		{
			now_cards.erase(iter);
			break;
		}
	}
}

void ShowCards(const CardInfo &cards)
{
	for (int i = 0; i < cards.number_size(); ++i)
	{
		int index = cards.number(i);
		ShowCard(index);
		AddCard(index);
		std::cout << " ";
	}
	std::cout << std::endl;
}

void onRoomMessage(const RoomMsg &msg)
{
	if (msg.type() == RoomMsg::ListResult)
	{
		std::cout << "Room List (total:" << msg.room_info_size() << ")" << std::endl;
		RoomInfo room;
		for (int i = 0; i < msg.room_info_size(); ++i)
		{
			room = msg.room_info(i);
			std::cout << "  " << i << " - " << room.number() << " : " << room.name();
			if (room.locked())
				std::cout << " - locked";
			std::cout << std::endl;
		}
	}
	else if (msg.type() == RoomMsg::LoginResult)
	{
		if (!msg.has_login_result())
		{
			std::cout << "create or login failed." << std::endl;
			return;
		}
		const LoginResult &result = msg.login_result();
		std::cout << "my user id: " << result.myid() << std::endl;
		const StatusInfo &info = result.status_info();
		if (info.type() == StatusInfo::Room)
		{
			const RoomDetail &detail = info.room_detail();
			std::cout << "Room:" << std::endl;
			std::cout << " id      - " << detail.number() << std::endl;
			std::cout << " name    - " << detail.name() << std::endl;
			std::cout << " timeout - " << detail.timeout() << std::endl;
			std::cout << " state   - " << (detail.state() == RoomDetail::Wait ? "wait" : "running") << std::endl;
		}
	}
}

void onChatMessage(const ChatMsg &msg)
{
	std::cout << "[" << getTime(msg.time()) << "]" <<
		"[" << msg.userid() << "]: " << msg.content() << std::endl;
	return;
}

void onGameMessage(const GameMsg &msg)
{
	if (msg.type() == GameMsg::Status && msg.has_status_info())
	{
		const StatusInfo &status = msg.status_info();
		if (status.type() == StatusInfo::User && status.user_info_size() >= 0)
		{
			std::cout << std::flush;
			std::cout << "User Info Updated:" << std::endl;
			std::stringstream ss;
			for (int i = 0; i < status.user_info_size(); ++i)
			{
				ss.str("");
				const UserInfo &user = status.user_info(i);
				switch (user.state())
				{
				case UserInfo::idle:
					ss << "idle";
					break;
				case UserInfo::ready:
					ss << "ready";
					break;
				case UserInfo::uno:
					ss << "uno";
					break;
				default:
					break;
				}
				std::cout << " " << user.id() << ": " << user.name() << " - " 
					<< user.card_count() << " - " << ss.str() << std::endl;
			}
		}
		else if (status.type() == StatusInfo::Room && status.has_room_detail())
		{
			const RoomDetail &detail = status.room_detail();
			std::cout << "Room:" << std::endl;
			std::cout << " id      - " << detail.number() << std::endl;
			std::cout << " name    - " << detail.name() << std::endl;
			std::cout << " timeout - " << detail.timeout() << std::endl;
			std::cout << " state   - " << (detail.state() == RoomDetail::Wait ? "wait" : "running") << std::endl;
		}
		else if (status.type() == StatusInfo::Play && status.has_play_state())
		{
			const PlayState &play = status.play_state();
			std::cout << "game play status: " << std::endl;
			std::cout << "now card: "; 
			ShowCard(play.now_card()); 
			std::cout << std::endl;
			std::cout << "now turn: " << play.now_turn() << std::endl;
			std::cout << "next turn: " << play.next_turn() << std::endl;
			std::cout << "left card: " << play.left_card() << std::endl;
		}
	}
	else if (msg.type() == GameMsg::DrawCardResult && msg.has_card_info())
	{
		const CardInfo &cards = msg.card_info();
		std::cout << "get " << cards.count() << " cards." << std::endl;
		ShowCards(cards);
	}
	else if (msg.type() == GameMsg::PlayCardResult)
	{
		if (msg.has_card_info())
		{
			const CardInfo &cards = msg.card_info();
			std::cout << "play wrong card, been punished." << std::endl;
			std::cout << "get " << cards.count() << " cards" << std::endl;
			ShowCards(cards);
		}
		else
		{
			std::cout << "play card success." << std::endl;
		}
	}
}

void doDispatch(std::string data)
{
	UNOMsg msg;
	if (!Decode(msg, data))
		return;
	bool bb = (msg.sequence() == 0);
	std::stringstream ss;
	if (!bb)
		ss << "reply seq:" << msg.sequence();
	LOG(debug) << "receive " << (bb ? "broadcast" : ss.str()); 
	switch (msg.type())
	{
	case UNOMsg::Chat:
		if (msg.has_chat_msg())
			onChatMessage(msg.chat_msg());
		break;
	case UNOMsg::Room:
		if (msg.has_room_msg())
			onRoomMessage(msg.room_msg());
		break;
	case UNOMsg::Game:
		if (msg.has_game_msg())
			onGameMessage(msg.game_msg());
		break;
	case UNOMsg::Error:
		LOG(warning) << "server reply error: " << msg.error_msg();
		break;
	default:
		break;
	}	
}

std::queue<std::string> msg_queue;
Semaphore queue_active(0);
bool quitting = false;
boost::shared_mutex mutex;
void handle_message()
{
	while (true)
	{
		queue_active.wait();
		if (quitting)
			break;
		readlock lock(mutex);
		if (msg_queue.empty())
			continue;

		doDispatch(msg_queue.front());
		upgradelock uplock(lock);
		msg_queue.pop();
	}
}

boost::thread thread(handle_message);
void onMessage(std::string data, SessionPtr session)
{
	writelock lock(mutex);
	msg_queue.push(data);
	queue_active.signal();
}

