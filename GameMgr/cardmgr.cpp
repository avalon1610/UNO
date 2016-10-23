#include "cardmgr.h"
#include "log.hpp"
#include "logic.h"
#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>

USING_NAMESPACE

CardMgr::CardMgr() : now_card_(0)
{
	reset();
}

void CardMgr::reset(UserCardsMap *user_cards)
{
	for (int i = 0; i < total_card_number; ++i)
	{
		deck_[i] = i;
	}

	std::list<int> excludes;
	if (user_cards != NULL)
	{
		UserCardsMap::iterator iter;
		std::list<int>::iterator iter_card;
		for (iter = user_cards->begin(); iter != user_cards->end(); ++iter)
		{
			for (iter_card = iter->second.begin(); iter_card != iter->second.end(); ++iter_card)
			{
				excludes.push_back(*iter_card);
			}
		}
	}
	shuffle(2, excludes);
}

CardMgr::~CardMgr()
{

}

void CardMgr::shuffle(int times, std::list<int> excludes)
{
	LOG(info) << "shuffling card...";
	int temp;
	int index;
	int i = 0;
	if (excludes.size() != 0)
	{
		std::list<int>::iterator iter;
		for (iter = excludes.begin(); iter != excludes.end(); ++iter)
		{
			deck_[i++] = *iter;
		}
	}

	boost::random::random_device rng;
	if (i > total_card_number - 1)
	{
		LOG(warning) << "no more available card";
		now_card_ = -1;
		return;
	}
	boost::random::uniform_int_distribution<> dist(i , total_card_number - 1);
	now_card_ = i;
	while (times--)
	{
		for (; i < total_card_number; ++i)
		{
			index = dist(rng);
			if (i == index)
				continue;
			temp = deck_[i];
			deck_[i] = deck_[index];
			deck_[index] = temp;
		}
	}
	
	// test
	/*
	index = 0;
	for (int j = 0; j < total_card_number; ++j)
	{
		std::cout << deck_[j] << " ";
		if (index >= 10)
		{
			std::cout << std::endl;
			index = 0;
		}
		++index;
	}
	std::cout << std::endl;
	*/
	LOG(info) << "done.";
};

std::string CardMgr::SendCard(int count, int64 sequence, std::list<int> &out_cards, UserCardsMap *user_cards)
{
	CardInfo *card = new CardInfo;
	card->set_count(count);

	GetCard(count, out_cards, user_cards);
	if (now_card_ == -1)
	{
		// card exhaust, end game
		return "";
	}
	std::list<int>::iterator iter;
	for (iter = out_cards.begin(); iter != out_cards.end(); ++iter)
	{
		card->add_number(*iter);
	}

	GameMsg *gmsg = new GameMsg;
	gmsg->set_type(GameMsg::DrawCardResult);
	gmsg->set_allocated_card_info(card);
	std::string data;
	if (!Encode(gmsg, data, sequence))
		LOG(error) << __FUNCTION__ << " encode failed.";
	return data;
}

int CardMgr::LeftCard()
{
	readlock lock(card_mutex_);
	return total_card_number - now_card_ - 1;
}

void CardMgr::GetCard(int count,std::list<int> &out_cards, UserCardsMap *user_cards)
{
	out_cards.clear();
	int sent = 0;
	writelock lock(card_mutex_);
	if (now_card_ == -1)
		return;
	while (sent < count)
	{
		if (total_card_number == now_card_ + 1) // all card used
			reset(user_cards);
		out_cards.push_back(deck_[now_card_++]);
		++sent;
	}
}

int CardMgr::GetTopCard()
{
	writelock lock(card_mutex_);
	if (now_card_ == -1)
		return -1;
	if (total_card_number == now_card_ + 1)
		reset();

	return deck_[now_card_++];
}
