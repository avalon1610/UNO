// ConsoleClient.cpp : Defines the entry point for the console application.
//
#include "defines.h"
#include "../Network/network.h"
#include "../Common/common.h"
#ifdef WIN32
#include <Windows.h>
#endif
#include <iostream>
USING_NAMESPACE

void wait()
{
#ifdef WIN32
	MSG msg;
	while (GetMessage(&msg, 0, NULL, NULL))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
#endif
}

typedef void (*Function)(std::string);
struct Command
{
	std::string cmd;
	Function func;
	std::string desc;
};


void GetGameRoomList(std::string);
void CreateRoom(std::string);
void Help(std::string);
void Ready(std::string);
void Login(std::string);
void Play(std::string);
void Play2(std::string);
void Draw(std::string);
void Show(std::string);
void Done(std::string);
void Uno(std::string);
void Doubt(std::string);
void Color(std::string);
void Sequence(std::string)
{
	for (unsigned int i = 0; i < 10; ++i)
	{
		std::cout << i << ":" << generateSequence() << std::endl;
	}
}

Command cmdlist[] = 
{
	{"ls" ,		GetGameRoomList,	"get room list"					},
	{"create",	CreateRoom,			"create a game room"			},
	{"login",	Login,				"login to room 1"				},	
	{"ready",	Ready,				"get ready for game"			},	
	{"help",	Help ,				"print this help"				},
	{"exit",	NULL,				"exit program"					},
	{"seq",		Sequence,			"test generate sequence"		},	
	{"play",	Play,				"play 1 card"					},	
	{"play2",	Play2,				"play 2 card"					},
	{"draw",	Draw,				"draw card"						},
	{"show",	Show,				"show cards in my hand"			},
	{"done",	Done,				"end this turn"					},
	{"uno",		Uno,				"yell UNO"						},
	{"doubt",	Doubt,				"doubt someone"					},
	{"color",	Color,				"send a color info"				}
};

void Help(std::string s)
{
	for (unsigned int i = 0; i < sizeof(cmdlist)/sizeof(Command); ++i)
	{
		std::cout << cmdlist[i].cmd << " - " << cmdlist[i].desc << std::endl;
	}
}

void onMessage(std::string, SessionPtr);
extern boost::thread thread;
extern bool quitting;
extern Semaphore queue_active;
int main(int argc, char *argv[])
{
	// todo: ip and port config
	std::string ip = "127.0.0.1";
	int port = 7777;
	Network::Run<Client>(port, ip);
	Network::Register(onMessage);
	std::string cmd;
	std::string param = "";
	int i;
	char command[32] = {0};
	while (true)
	{
		std::cin.getline(command, sizeof(command)/sizeof(char), '\n');
		cmd = command;
		if ((i = cmd.find(" ")) != std::string::npos)
		{
			param = cmd.substr(i + 1);
			cmd = cmd.substr(0, i);
		}
		if (cmd == "exit")
			break;
		for (unsigned int i = 0; i < sizeof(cmdlist)/sizeof(Command); ++i)
		{
			if (cmdlist[i].cmd == cmd && cmdlist[i].func != NULL)
			{
				cmdlist[i].func(param);
				break;
			}
		}
	}
	quitting = true;
	queue_active.signal();
	thread.join();
	Network::Shutdown();
	return 0;
}

