#include "logic.h"
#include "gamemgr.h"
#include "log.hpp"
#include "usermgr.h"
#include "cardmgr.h"

USING_NAMESPACE

Logic::Logic(GameMgr *game, UserMgr *user, CardMgr* card) : running_(false), reverse_(false),
	defer_punish_count_(0), drawn_card_(-1), wait_for_change_color_(false), now_card_(-1),
	wait_for_plus4_assign_(false), plus4_user_(0), previous_card_(-1), now_turn_(0), 
	double_card_(false), intercepted_(false)
{
	gamemgr = game;
	usermgr = user;
	cardmgr = card;
	Session::subscribe(close,1,boost::bind(&Logic::onSessionClose, this, _1));
}

Logic::~Logic()
{

}

void Logic::handle_message(const GameMsg &msg, SessionPtr session, int64 seq)
{
	int32 id;
	if (session == NULL)	// timeout fake msg
		id = now_turn_;
	else
	{
		id = gamemgr->GetUserID(session);
		if (id == 0)
		{
			LOG(error) << "get user id failed from session failed.";
			return;
		}
	}

	UserInfo user;
	if (!usermgr->GetUser(id, &user))
	{
		LOG(error) << "get user info failed, id:" << id;
		return;
	}

	if (gamemgr->isRunning())
		gamemgr->ResetTimer();

	if (wait_for_plus4_assign_ || wait_for_change_color_)
	{
		if (msg.type() == GameMsg::PlayCard || msg.type() == GameMsg::DrawCard ||
			msg.type() == GameMsg::Done)
		{
			gamemgr->SystemBroadcast(id, TextSpecification[0]);
			Punish(id, session, seq, 2);
			return;
		}
	}

	switch (msg.type())
	{
	case GameMsg::Ready:
		if (!gamemgr->isRunning())	
		{
			usermgr->SetUser(id, UserInfo::ready);
			gamemgr->Broadcast(boost::bind(&UserMgr::NotifyUser, usermgr, id));

			// if user >= 2, and all this guys has been ready, game start!
			if (usermgr->GetUserCount() >= 2 && usermgr->isEveryoneReady())
			{
				LOG(info) << "room: " << gamemgr->name() << " game start !!!!";
				gamemgr->Start();

				int count = 7;
				// send 7 card to everyone
				std::list<int32> user_list = usermgr->GetUserList();
				std::list<int32>::iterator iter;
				std::list<int> cards;
				std::list<int>::iterator card_iter;
				for (iter = user_list.begin(); iter != user_list.end(); ++iter)
				{
					if (gamemgr->SendToUser(*iter, boost::bind(&CardMgr::SendCard,
						cardmgr, count, 0, boost::ref(cards), &user_cards_)))
					{
						LOG(debug) << "user: " << *iter << " have " << cards.size() << " cards";
						for (card_iter = cards.begin(); card_iter != cards.end(); ++card_iter)
						{
							std::cout << *card_iter << " ";
						}
						if (cards.size())
							std::cout << std::endl;
						user_cards_[*iter] = boost::move(cards);
						usermgr->SetUser(*iter, count);
					}
				}
				
				gamemgr->Broadcast(boost::bind(&UserMgr::NotifyUser, usermgr, 0));
				// get top card as first one
				do 
				{
					now_card_ = cardmgr->GetTopCard();
				} while (isFunctional(now_card_) || isBlack(now_card_));

				// set up first turn and order
				now_turn_ = usermgr->GetFirstUser();
				gamemgr->Broadcast(boost::bind(&Logic::SendPlayState, this));
			}
		}
		break;
	case GameMsg::PlayCard:
	{
		if (!msg.has_card_info())
		{
			LOG(error) << "playcard msg do not have card info";
			return;
		}
		if (!gamemgr->isRunning())
			return;
		const CardInfo &cards = msg.card_info();

		if (cards.count() != 2 && cards.count() != 1)
		{
			LOG(error) << "cards count not 1 or 2, impossible.";
			return;
		}

		if (cards.count() == 2)
		{
			if (cards.number_size() != 2)
			{
				LOG(error) << "cards count uncorrect.";
				return;
			}
			
			if (!isSameCard(cards.number(0), cards.number(1)))
			{
				gamemgr->SystemBroadcast(id, TextSpecification[1]);
				Punish(id, session, seq, 2);
				return;
			}
			
			if (isBlack(cards.number(0)))
			{
				gamemgr->SystemBroadcast(id, TextSpecification[2]);
				Punish(id, session, seq, 2);
				return;
			}

			if (drawn_card_ != -1)
			{
				gamemgr->SystemBroadcast(id, TextSpecification[3]);
				Punish(id, session, seq, 2);
				return;
			}
		}

		// check if user have this cards
		UserCardsMap::iterator iter = user_cards_.find(id);
		if (iter == user_cards_.end())
		{
			LOG(error) << "can not find id " << id << " in user card map";
			return;
		}

		std::list<int>::iterator iter_card;
		bool found_1 = false;
		bool found_2 = false;
		for (iter_card = iter->second.begin(); iter_card != iter->second.end(); ++iter_card)
		{
			if (*iter_card == cards.number(0))
				found_1 = true;
			if (cards.count() == 2 && *iter_card == cards.number(1))
				found_2 = true;
		}

		if (!found_1 || (cards.count() == 2 && !found_2))
		{
			gamemgr->SystemBroadcast(id, TextSpecification[4]);
			Punish(id, session, seq, 2);
			return;
		}
		
		if (cards.count() == 2)
			double_card_ = true;

		int card = cards.number(0);

		if (now_turn_ != id && (!isSameCard(card, now_card_) || isBlack(card) || drawn_card_ != -1))  // interception
		{
			gamemgr->SystemBroadcast(id, TextSpecification[5]);
			Punish(id, session, seq, 2);
			return;
		}
		else if (!isSameColor(card, now_card_) && // normal
				 !isSameNumber(card, now_card_) &&
				 !isBlack(card))
		{
			gamemgr->SystemBroadcast(id, TextSpecification[6]);
			Punish(id, session, seq, 2);
			return;
		}

		if (drawn_card_ != -1)
		{
			if (!isSameCard(drawn_card_, card) || isFunctional(card))
			{
				gamemgr->SystemBroadcast(id, TextSpecification[7]);
				Punish(id, session, seq, 2);
				return;
			}
		}

		if (now_turn_ != id)
			intercepted_ = true;
		now_turn_ = id;

		if (defer_punish_count_ != 0 && 
			(plus4_user_ || CardCollection[card].CardFunction != Plus2) && 
			CardCollection[card].CardFunction != Plus4)
		{
			gamemgr->SystemBroadcast(id, TextSpecification[8]);
			Punish(id, session, seq, 2);
			return;
		}

		if (isFunctional(card))
		{
			switch (CardCollection[card].CardFunction)
			{
			case Reverse:
				if (cards.count() == 1)
					reverse_ = (reverse_ ? false : true);
				break;
			case Skip:
				now_turn_ = usermgr->WhoIsNext(now_turn_, reverse_);
				if (cards.count() == 2)
					now_turn_ = usermgr->WhoIsNext(now_turn_, reverse_);
				break;
			case Plus2:
				defer_punish_count_ += 2 * cards.count();
				break;
			case Change:
				wait_for_change_color_ = true;
				gamemgr->SystemBroadcast(id, TextSpecification[21]);
				break;
			case Plus4:
				defer_punish_count_ += 4;
				wait_for_plus4_assign_ = true;
				gamemgr->SystemBroadcast(id, TextSpecification[22]);
				break;
			default:
				break;
			}
		}

		Pass(id, session, seq, cards);
		previous_card_ = now_card_;
		now_card_ = card;
		if (!wait_for_change_color_ && !wait_for_plus4_assign_)
			now_turn_ = usermgr->WhoIsNext(now_turn_, reverse_);
		gamemgr->Broadcast(boost::bind(&Logic::SendPlayState, this));
		double_card_ = false;
		intercepted_ = false;
		break;
	}
	case GameMsg::DrawCard:
	{
		if (!gamemgr->isRunning())
			return;
		if (now_turn_ != id)
		{
			gamemgr->SystemBroadcast(id, TextSpecification[9]);
			Punish(id, session, seq, 2);
			return;
		}

		int draw_count = 1;
		if (defer_punish_count_ != 0)
			draw_count = defer_punish_count_;
		std::list<int> out_cards;
		cardmgr->GetCard(draw_count, out_cards, &user_cards_);
		std::list<int>::iterator iter;
		CardInfo *cards = new CardInfo;
		for (iter = out_cards.begin(); iter != out_cards.end(); ++iter)
		{
			cards->add_number(*iter);
			user_cards_[id].push_back(*iter);
		}
		if (user_cards_[id].size() > 1)
			usermgr->SetUser(id, UserInfo::ready);
		cards->set_count(out_cards.size());
		SendDrawCardResult(session, seq, cards);
		usermgr->SetUser(id, user_cards_[id].size());
		gamemgr->Broadcast(boost::bind(&UserMgr::NotifyUser, usermgr, id));
		if (defer_punish_count_)
		{
			defer_punish_count_ = 0;
			if (plus4_user_)
			{
				plus4_user_ = 0;
				wait_for_change_color_ = true;
				gamemgr->SystemBroadcast(id, TextSpecification[21]);
			}
			else
			{
				now_card_ = GetAnyCard(CardCollection[now_card_].color);
				now_turn_ = usermgr->WhoIsNext(id, reverse_);
				gamemgr->Broadcast(boost::bind(&Logic::SendPlayState, this));
			}
		}
		else
		{
			drawn_card_ = out_cards.front();
		}
		break;
	}
	case GameMsg::Done:
		if (!gamemgr->isRunning())
			return;
		if (drawn_card_ == -1)
		{
			gamemgr->SystemBroadcast(id, TextSpecification[10]);
			Punish(id, session, seq, 2);
			return;
		}
		now_turn_ = usermgr->WhoIsNext(id, reverse_);
		gamemgr->Broadcast(boost::bind(&Logic::SendPlayState, this));
		break;
	case GameMsg::UNO:
	{
		if (!gamemgr->isRunning())
			return;
		UserInfo user;
		if (!usermgr->GetUser(id, &user))
		{
			LOG(error) << "uno msg get user error.";
			return;
		}
		if (user.card_count() != user_cards_[id].size())
		{
			LOG(error) << "user card count not sync in usermgr and logic";
			return;
		}
		if (user.card_count() != 1)
		{
			gamemgr->SystemBroadcast(id, TextSpecification[11]);
			Punish(id, session, seq, 2);
			return;
		}
		usermgr->SetUser(id, UserInfo::uno);
		gamemgr->Broadcast(boost::bind(&UserMgr::NotifyUser, usermgr, id));
		break;
	}
	case GameMsg::Doubt:
		if (!gamemgr->isRunning())
			return;
		if (msg.has_doubt_info())	// doubt for uno
		{
			const DoubtInfo &doubt_info = msg.doubt_info();
			int doubt_id = doubt_info.user_id();
			UserInfo user;
			if (!usermgr->GetUser(doubt_id, &user))
			{
				LOG(error) << "user " << id << " not fount in doubt process";
				return;
			}

			if (user.card_count() != user_cards_[doubt_id].size())
			{
				LOG(error) << "user card count not sync in usermgr and logic";
				return;
			}

			if (user.card_count() == 1 && user.state() != UserInfo::uno)
			{
				LOG(info) << "doubt uno succeed";
				std::list<int> out_cards;
				gamemgr->SendToUser(doubt_id, boost::bind(&CardMgr::SendCard, cardmgr, 2, seq,
					boost::ref(out_cards), &user_cards_));
				if (out_cards.size() == 0)
				{
					gamemgr->Stop();
					return;
				}
				std::list<int>::iterator iter;
				for (iter = out_cards.begin(); iter != out_cards.end(); ++iter)
				{
					user_cards_[doubt_id].push_back(*iter);
				}

				usermgr->SetUser(doubt_id, UserInfo::ready);
				usermgr->SetUser(doubt_id, user_cards_[doubt_id].size());
				gamemgr->Broadcast(boost::bind(&UserMgr::NotifyUser, usermgr, id));
				gamemgr->SystemBroadcast(doubt_id, TextSpecification[17]);
			}
			else
			{
				gamemgr->SystemBroadcast(id, TextSpecification[12]);
				Punish(id, session, seq, 2);
				return;
			}
		}
		else	// doubt for +4
		{
			if (plus4_user_ == 0)
			{
				LOG(error) << "plus4 user is 0, this should never happen.";
				return;
			}
			const std::list<int> &plus_user_card = user_cards_[plus4_user_];
			std::list<int>::const_iterator iter;
			CardInfo *cards = new CardInfo;
			bool doubt_success = false;
			for (iter = plus_user_card.begin(); iter != plus_user_card.end(); ++iter)
			{
				cards->add_number(*iter);
				if (isSameNumber(previous_card_, *iter) || isSameNumber(previous_card_, *iter) ||
					CardCollection[*iter].CardFunction == Change)
				{
					doubt_success = true;
				}
			}
			cards->set_count(plus_user_card.size());
			GameMsg *gmsg = new GameMsg;
			gmsg->set_type(GameMsg::Black);
			gmsg->set_allocated_card_info(cards);
			std::string data;
			if (Encode(gmsg, data, seq))
				session->send(data);
			if (doubt_success)
			{
				std::list<int> out_cards;
				gamemgr->SendToUser(plus4_user_, boost::bind(&CardMgr::SendCard, cardmgr, 
					defer_punish_count_, 0, boost::ref(out_cards), &user_cards_));	
				if (out_cards.size() == 0)
				{
					gamemgr->Stop();
					return;
				}
				std::list<int>::iterator iter;
				for (iter = out_cards.begin(); iter != out_cards.end(); ++iter)
				{
					user_cards_[plus4_user_].push_back(*iter);
				}
				
				if (user_cards_[plus4_user_].size() > 1)
					usermgr->SetUser(id, UserInfo::ready);
				usermgr->SetUser(plus4_user_, user_cards_[plus4_user_].size());
				gamemgr->Broadcast(boost::bind(&UserMgr::NotifyUser, usermgr, plus4_user_));
				now_turn_ = plus4_user_;
				gamemgr->Broadcast(boost::bind(&Logic::SendPlayState, this));
				gamemgr->SystemBroadcast(id, TextSpecification[19]);
			}
			else
			{
				defer_punish_count_ += 2;
				gamemgr->SystemBroadcast(id, TextSpecification[14]);
				Punish(id, session, seq, defer_punish_count_);
			}
			wait_for_change_color_ = true;
			gamemgr->SystemBroadcast(id, TextSpecification[21]);
			defer_punish_count_ = 0;
			plus4_user_ = 0;
		}
		break;
	case GameMsg::Black:
		if (!gamemgr->isRunning())
			return;

		// todo: comment this to check protobuf-net double enum issue
		if (!wait_for_change_color_ && !wait_for_plus4_assign_)
		{
			gamemgr->SystemBroadcast(id, TextSpecification[20]);
			Punish(id, session, seq, 2);
			return;
		}

		if (msg.has_color_info())
		{
			if (wait_for_plus4_assign_)
			{
				gamemgr->SystemBroadcast(id, TextSpecification[15]);
				Punish(id, session, seq, 2);
				return;
			}
			int index = GetAnyCard(msg.color_info());
			now_card_ = index;
			now_turn_ = usermgr->WhoIsNext(now_turn_, reverse_);
			wait_for_change_color_ = false;
			gamemgr->Broadcast(boost::bind(&Logic::SendPlayState, this));
		}
		else if (msg.has_doubt_info())
		{
			if (wait_for_change_color_)
			{
				gamemgr->SystemBroadcast(id, TextSpecification[16]);
				Punish(id, session, seq, 2);
				return;
			}
			int assign_id = msg.doubt_info().user_id();
			now_turn_ = assign_id;
			wait_for_plus4_assign_ = false;
			plus4_user_ = id;
			gamemgr->SendToUser(assign_id, boost::bind(&Logic::SendBlackInfo, this, assign_id));
			gamemgr->Broadcast(boost::bind(&Logic::SendPlayState, this));
			gamemgr->SystemBroadcast(assign_id, TextSpecification[18]);
		}
		break;
	case GameMsg::Timeout:
	{
		if (wait_for_plus4_assign_)
		{
			wait_for_plus4_assign_ = false;
			now_card_ = GetRandomColor();
		}

		int punished_count = 2;
		if (defer_punish_count_ != 0)
		{
			punished_count += defer_punish_count_;
			defer_punish_count_ = 0;
			if (plus4_user_ != 0)
			{
				now_card_ = GetRandomColor();
				plus4_user_ = 0;
			}
		}
		
		if (wait_for_change_color_)
		{
			wait_for_change_color_ = false;
			now_card_ = GetRandomColor();
		}

		std::list<int> out_cards;
		gamemgr->SendToUser(now_turn_, boost::bind(&CardMgr::SendCard, cardmgr, 
			punished_count, 0, boost::ref(out_cards), &user_cards_));
		if (out_cards.size() == 0)
		{
			gamemgr->Stop();
			return;
		}
		
		std::list<int>::iterator iter;
		for (iter = out_cards.begin(); iter != out_cards.end(); ++iter)
		{
			user_cards_[now_turn_].push_back(*iter);
		}
		
		if (user_cards_[now_turn_].size() > 1)
			usermgr->SetUser(id, UserInfo::ready);
		usermgr->SetUser(now_turn_, user_cards_[now_turn_].size());
		gamemgr->Broadcast(boost::bind(&UserMgr::NotifyUser, usermgr, now_turn_));
		gamemgr->SystemBroadcast(now_turn_, TextSpecification[13]);
		now_turn_ = usermgr->WhoIsNext(now_turn_, reverse_);
		gamemgr->Broadcast(boost::bind(&Logic::SendPlayState, this));
		break;
	}
	default:
		break;
	}
}

int Logic::GetAnyCard(Color color)
{
	int index = -1;
	switch (color)
	{
	case Red:
		index = 108;
		break;
	case Blue:
		index = 109;
		break;
	case Green:
		index = 110;
		break;
	case Yellow:
		index = 111;
		break;
	default:
		break;
	}

	return index;
}

int Logic::GetAnyCard(GameMsg::ColorInfo color)
{
	int index = -1;
	switch (color)
	{
	case GameMsg::Red:
		index = 108;
		break;
	case GameMsg::Blue:
		index = 109;
		break;
	case GameMsg::Green:
		index = 110;
		break;
	case GameMsg::Yellow:
		index = 111;
		break;
	default:
		LOG(error) << "black operation color info error";
		break;
	}

	return index;
}

int Logic::GetRandomColor()
{
	return generateNumberInRange(108, 111);
}

std::string Logic::SendBlackInfo(int32 user_id)
{
	DoubtInfo *doubt = new DoubtInfo;
	doubt->set_user_id(user_id);
	GameMsg *gmsg = new GameMsg;
	gmsg->set_allocated_doubt_info(doubt);
	gmsg->set_type(GameMsg::Black);
	std::string data = "";
	if (!Encode(gmsg, data, 0))
		LOG(error) << __FUNCTION__ << " encode failed.";
	return data;
}

void Logic::SendDrawCardResult(SessionPtr session, int64 seq, CardInfo *cardinfo)
{
	GameMsg *gmsg = new GameMsg;
	gmsg->set_type(GameMsg::DrawCardResult);
	if (cardinfo != NULL)
		gmsg->set_allocated_card_info(cardinfo);
	std::string data;
	if (Encode(gmsg, data, seq))
		session->send(data);
}

void Logic::SendPlayCardResult(SessionPtr session, int64 seq, CardInfo *cardinfo)
{
	GameMsg *gmsg = new GameMsg;
	gmsg->set_type(GameMsg::PlayCardResult);
	if (cardinfo != NULL)
		gmsg->set_allocated_card_info(cardinfo);
	std::string data;
	if (Encode(gmsg, data, seq))
		session->send(data);
}

void Logic::Pass(int32 id, SessionPtr session, int64 seq, const CardInfo &cards)
{
	UserCardsMap::iterator iter = user_cards_.find(id);
	if (iter == user_cards_.end())
	{
		LOG(error) << __FUNCTION__ << " can not found id " << id;
		return;
	}
	
	std::list<int>::iterator iter_card;
	for (int i = 0; i < cards.number_size(); ++i)
	{
		for (iter_card = iter->second.begin(); iter_card != iter->second.end(); ++iter_card)
		{
			if (*iter_card == cards.number(i))
			{
				iter->second.erase(iter_card);
				break;
			}
		}
	}

	SendPlayCardResult(session, seq);

	// check last card
	if (user_cards_[id].size() == 0)
	{
		if (isBlack(cards.number(0)))
			Punish(id, session, seq, 2);
		else
			gamemgr->Stop();
		return;
	}

	usermgr->SetUser(id, iter->second.size());
	gamemgr->Broadcast(boost::bind(&UserMgr::NotifyUser, usermgr, id));
}

void Logic::Punish(int32 id, SessionPtr session, int64 seq, int count)
{
	UserCardsMap::iterator iter_user = user_cards_.find(id);
	if (iter_user == user_cards_.end())
	{
		LOG(error) << __FUNCTION__ << " can not found id from user cards map";
		return;
	}

	std::list<int> cards;
	cardmgr->GetCard(count, cards, &user_cards_);

	std::list<int>::iterator iter;
	CardInfo *card_info = new CardInfo;
	for (iter = cards.begin(); iter != cards.end(); ++iter)
	{
		card_info->add_number(*iter);
		punished_cards_[iter_user->first].push_back(*iter);
	}
	card_info->set_count(count);
	SendPlayCardResult(session, seq, card_info);
	usermgr->SetUser(id, iter_user->second.size());
	gamemgr->Broadcast(boost::bind(&UserMgr::NotifyUser, usermgr, id));
}

std::string Logic::SendPlayState()
{
	// merge punished card
	if (punished_cards_.size() != 0)
	{
		UserCardsMap::iterator iter_p;
		UserCardsMap::iterator iter_u;
		std::list<int>::iterator iter_card;
		for (iter_p = punished_cards_.begin(); iter_p != punished_cards_.end(); ++iter_p)
		{
			iter_u = user_cards_.find(iter_p->first);
			if (iter_u != user_cards_.end())
			{
				for (iter_card = iter_p->second.begin(); iter_card != iter_p->second.end(); ++iter_card)
				{
					iter_u->second.push_back(*iter_card);
				}
			}
		}

		punished_cards_.clear();
	}
	drawn_card_ = -1;
	StatusInfo *status = new StatusInfo;
	status->set_type(StatusInfo::Play);
	PlayState *play = new PlayState;
	play->set_now_card(now_card_);
	play->set_now_turn(now_turn_);
	play->set_double_card(double_card_);
	play->set_intercepted(intercepted_);
	play->set_next_turn(usermgr->WhoIsNext(now_turn_, reverse_));
	play->set_left_card(cardmgr->LeftCard());
	status->set_allocated_play_state(play);
	GameMsg *gmsg = new GameMsg;
	gmsg->set_type(GameMsg::Status);
	gmsg->set_allocated_status_info(status);
	std::string data;

	if (!Encode(gmsg, data, 0))
		LOG(error) << __FUNCTION__ << " encode failed.";
	return data;
}

void Logic::onSessionClose(SessionPtr session)
{
	if (!gamemgr->isRunning())
		return;
	
	// found disconnect user
	UserCardsMap::iterator iter;
	for (iter = user_cards_.begin(); iter != user_cards_.end(); ++iter)
	{
		if (!usermgr->GetUser(iter->first))
		{
			user_cards_.erase(iter);
			break;
		}	
	}

	if (wait_for_change_color_ || wait_for_change_color_)
	{
		now_card_ = GetRandomColor();
		wait_for_change_color_ = false;
		wait_for_plus4_assign_ = false;
	}
	if (defer_punish_count_ != 0)
	{
		defer_punish_count_ = 0;
		if (plus4_user_ != 0)
			plus4_user_ = 0;
	}

	LOG(debug) << "logic session close";
	now_turn_ = usermgr->WhoIsNext(now_turn_, reverse_);
	gamemgr->Broadcast(boost::bind(&Logic::SendPlayState, this));
}

std::string Logic::CalculateScore()
{
	UserCardsMap::iterator iter;
	std::list<int>::iterator iter_card;
	int score = 0;
	ScoreMsg *smsg = new ScoreMsg;
	ScoreInfo *sinfo;
	for (iter = user_cards_.begin(); iter != user_cards_.end(); ++iter)
	{
		score = 0;
		sinfo = smsg->add_score_info();
		sinfo->set_userid(iter->first);
		for (iter_card = iter->second.begin(); iter_card != iter->second.end(); ++iter_card)
		{
			score += getScore(*iter_card);
		}
		sinfo->set_score(score);
	}

	std::string data = "";
	if (!Encode(smsg, data, 0))
		LOG(error) << __FUNCTION__ << " encode failed.";
	return data;
}

void Logic::reset()
{
	if (usermgr->GetUserCount() > 1)
		gamemgr->Broadcast(boost::bind(&Logic::CalculateScore, this));
	running_ = false;
	reverse_ = false;
	defer_punish_count_ = 0;
	drawn_card_ = -1;
	now_card_ = -1;
	double_card_ = false;
	intercepted_ = false;
	now_turn_ = 0;
	user_cards_.clear();
	wait_for_change_color_ = false;
	wait_for_plus4_assign_ = false;
	plus4_user_ = 0;
	previous_card_ = -1;
	gamemgr->Broadcast(boost::bind(&Logic::SendPlayState, this));
}
