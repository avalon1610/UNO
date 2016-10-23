// Card.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "../Common/Cards.h"
#include "Card.h"

USING_NAMESPACE

CARD_API bool GetCard(int index, int *color, int *number)
{
	if (index < 0 || index > sizeof(CardCollection)/sizeof(Card))
		return false;

	Card card = CardCollection[index];
	*color = card.color;
	*number = card.CardNumber;
	return true;
}

CARD_API bool IsPlus4(int index)
{
	return isPlus4(index);
}

CARD_API bool IsSame(int _1, int _2)
{
	return isSameCard(_1, _2);
}