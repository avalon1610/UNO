#ifndef _UNO_DISPATCHER_H__
#define _UNO_DISPATCHER_H__
#include "defines.h"
#include "singleton.hpp"
#include "../Common/common.h"
#include "../Network/session.h"
#include <boost/function.hpp>
#include <boost/asio.hpp>

NAMESPACE_PROLOG

typedef boost::function<void (UNOMsg *,SessionPtr,Timestamp)> MsgHandler;
SINGLETON_CLASS_BEGIN(Dispatcher)
public:
	virtual ~Dispatcher();
	typedef boost::asio::ip::tcp::endpoint endpoint;
	bool Register(const MsgHandler &, SessionPtr = NULL);	// game register
	static void Error(std::string error_msg, SessionPtr session, int64 seq = 0);
private:
	Dispatcher();
	void onMessage(const std::string &data,SessionPtr);
	void onSessionClose(SessionPtr session);
	typedef std::map<SessionPtr, MsgHandler> DispatcherMap;
	DispatcherMap dispatch_map_;	// game 
	MsgHandler room_handler_;	 // only one
	boost::shared_mutex map_mutex_;
SINGLETON_CLASS_END

NAMESPACE_EPILOG
#endif