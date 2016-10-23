#include "../Network/network.h"
#include "../RoomMgr/roommgr.h"
#include <stdio.h>
#ifdef WIN32
#include <Windows.h>
#endif
#include <boost/program_options.hpp>

USING_NAMESPACE
namespace po = boost::program_options;
void wait()
{
#ifdef WIN32
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
#endif
}

int main(int argc, char *argv[])
{
	po::options_description desc("Allowed options");
	desc.add_options()
		("help","produce help message")
		("max-room",po::value<int>(),"set max room count")
		("port",po::value<int>(),"server listen port");
	po::positional_options_description p;
	p.add("port", -1);
		
	int port = 7777;
	int max_room = 10;
	try
	{
		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
		po::notify(vm);
		if (vm.count("help"))
		{
			std::cout << desc << std::endl;
			return 1;
		}

		if (vm.count("max-room"))
		{
			max_room = vm["max-room"].as<int>();
			std::cout << "set max room: " << max_room << std::endl;
		}

		if (vm.count("port"))
		{
			port = vm["port"].as<int>();
		}
	}
	catch (po::error_with_option_name s)
	{
		std::cerr << s.what() << std::endl;
		return 1;
	}
	
	
	// start network 
	Network::Run<Server>(port);
	// start RoomManager
	RoomMgr::instance()->init(max_room);
	
	//wait();
	Network::Join();
}