#ifndef __UNO_LOGIC_H__
#define __UNO_LOGIC_H__
#include "defines.h"
#include "../Common/common.h"
#include "../Common/Cards.h"
#include "../Network/session.h"
#include <list>
#include <map>
#include <boost/noncopyable.hpp>

NAMESPACE_PROLOG

class GameMgr;
class UserMgr;
class CardMgr;
class UserCardsMap : public std::map<int32, std::list<int>> {};
class Logic : private boost::noncopyable
{
public:
	Logic(GameMgr *, UserMgr *, CardMgr *);
	virtual ~Logic();
	void handle_message(const GameMsg &, SessionPtr, int64);
	std::string SendPlayState();
	void reset();
private:
	void onSessionClose(SessionPtr session);
	std::string CalculateScore();
	void Punish(int32, SessionPtr session, int64, int count);
	void SendDrawCardResult(SessionPtr, int64, CardInfo *cardinfo = NULL);
	void SendPlayCardResult(SessionPtr, int64, CardInfo *cardinfo = NULL);
	void Pass(int32 id, SessionPtr session, int64 seq, const CardInfo &cards);
	std::string SendBlackInfo(int32 id);
	int GetAnyCard(GameMsg::ColorInfo color);
	int GetAnyCard(Color color);
	int GetRandomColor();
	GameMgr *gamemgr;
	UserMgr *usermgr;
	CardMgr *cardmgr;
	bool running_;
	int now_card_;
	bool double_card_;
	bool intercepted_;
	int32 now_turn_;
	bool reverse_;
	UserCardsMap user_cards_;
	UserCardsMap punished_cards_;
	int defer_punish_count_;
	int drawn_card_;
	bool wait_for_change_color_;
	bool wait_for_plus4_assign_;
	int32 plus4_user_;	// the one who play +4
	int previous_card_;
};



NAMESPACE_EPILOG

#endif