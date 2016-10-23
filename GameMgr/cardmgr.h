#ifndef _UNO_CARD_MGR_H__
#define _UNO_CARD_MGR_H__
#include "defines.h"
#include "../Common/common.h"
#include "../Common/Cards.h"
#include <string>


NAMESPACE_PROLOG
class UserCardsMap;
class CardMgr
{
public:
	CardMgr();
	
	virtual ~CardMgr();
	void reset(UserCardsMap *user_cards = NULL);
	std::string SendCard(int count, int64 sequence, std::list<int> &, UserCardsMap *);
	int LeftCard();
	void GetCard(int count,std::list<int> &out_cards, UserCardsMap *user_cards);
	int GetTopCard();
private:
	void shuffle(int times, std::list<int>);
	static const int total_card_number = sizeof(CardCollection)/sizeof(Card) - 4; // exclude 4 any card
	int now_card_;
	int deck_[total_card_number];
	boost::shared_mutex card_mutex_;

};

NAMESPACE_EPILOG
#endif