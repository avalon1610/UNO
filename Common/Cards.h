#ifndef __TREADSTONE_CARDS_H__
#define __TREADSTONE_CARDS_H__
#include "defines.h"

NAMESPACE_PROLOG

enum Color
{
	Red = 0,
	Blue,
	Green,
	Yellow,
	Black
};

enum Functional
{
	Reverse = 10,
	Skip,
	Plus2,
	Change,
	Plus4,
	Any
};

#ifdef WIN32
#pragma pack(push,1)
#endif
struct Card
{
	Color color;
	union
	{
		unsigned int num;
		Functional func;	
	} c;
};
#ifdef WIN32
#pragma pack(pop)
#endif

#define CardNumber c.num 
#define CardFunction c.func
#define isFunctional(i) (CardCollection[i].CardNumber >= 10)
#define isBlack(i) (CardCollection[i].color == Black)
#define isPlus4(i) (CardCollection[i].CardFunction == Plus4)
#define isSameColor(a, b) (CardCollection[a].color == CardCollection[b].color)
#define isSameNumber(a, b) (CardCollection[a].CardNumber == CardCollection[b].CardNumber)
#define isSameCard(a, b) (\
	(CardCollection[a].color != Black) && \
	(isSameColor(a, b)) && (isSameNumber(a, b)))
#define getScore(i)	\
	(!isFunctional(i)) ? CardCollection[i].CardNumber : (isBlack(i) ? 50 : 20)

const Card CardCollection[] =
{
	{Red, 	0}, {Red, 	1}, {Red, 	2}, {Red, 	3}, {Red, 	4},  
	{Red, 	5}, {Red, 	6}, {Red, 	7}, {Red, 	8}, {Red, 	9},
				{Red, 	1}, {Red, 	2}, {Red, 	3}, {Red, 	4},  
	{Red, 	5}, {Red, 	6}, {Red, 	7}, {Red, 	8}, {Red, 	9},

	{Blue, 	0}, {Blue, 	1}, {Blue, 	2}, {Blue, 	3}, {Blue, 	4},  
	{Blue, 	5}, {Blue, 	6}, {Blue, 	7}, {Blue, 	8}, {Blue, 	9},
				{Blue, 	1}, {Blue, 	2}, {Blue, 	3}, {Blue, 	4},  
	{Blue, 	5}, {Blue, 	6}, {Blue, 	7}, {Blue, 	8}, {Blue, 	9},

	{Green, 0}, {Green, 1}, {Green, 2}, {Green, 3}, {Green, 4},  
	{Green, 5}, {Green, 6}, {Green, 7}, {Green, 8}, {Green, 9},
				{Green, 1}, {Green, 2}, {Green, 3}, {Green, 4},  
	{Green, 5}, {Green, 6}, {Green, 7}, {Green, 8}, {Green, 9},

	{Yellow, 0}, {Yellow, 1}, {Yellow, 2}, {Yellow, 3}, {Yellow, 4},  
	{Yellow, 5}, {Yellow, 6}, {Yellow, 7}, {Yellow, 8}, {Yellow, 9},
				 {Yellow, 1}, {Yellow, 2}, {Yellow, 3}, {Yellow, 4},  
	{Yellow, 5}, {Yellow, 6}, {Yellow, 7}, {Yellow, 8}, {Yellow, 9},

	{Red, Reverse}, {Blue, Reverse}, {Green, Reverse}, {Yellow, Reverse},
	{Red, Reverse}, {Blue, Reverse}, {Green, Reverse}, {Yellow, Reverse},

	{Red, Skip}, {Blue, Skip}, {Green, Skip}, {Yellow, Skip},
	{Red, Skip}, {Blue, Skip}, {Green, Skip}, {Yellow, Skip},

	{Red, Plus2}, {Blue, Plus2}, {Green, Plus2}, {Yellow, Plus2},
	{Red, Plus2}, {Blue, Plus2}, {Green, Plus2}, {Yellow, Plus2},

	{Black, Change}, {Black, Plus4}, 
	{Black, Change}, {Black, Plus4},
	{Black, Change}, {Black, Plus4},
	{Black, Change}, {Black, Plus4},

	// should not in deck
	{Red, Any}, {Blue, Any}, {Green, Any}, {Yellow, Any} 
};

NAMESPACE_EPILOG
#endif