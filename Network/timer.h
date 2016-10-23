#ifndef _UNO_TIMER_H_
#define _UNO_TIMER_H_
#include "defines.h"
#include "network.h"
#include <boost/asio.hpp>
#include <boost/asio/system_timer.hpp>
#include <boost/chrono.hpp>

NAMESPACE_PROLOG

template<class WaitHandler>
class Timer
{
public:
	Timer(int second, WaitHandler handler) : second_(second),strand_(Network::io_service()),t_(NULL)
	{
		handler_ = handler;
	}

	~Timer()
	{
		stop();
	}

	void start()
	{
		if (t_ == NULL)
			t_ = new boost::asio::system_timer(Network::io_service(), boost::chrono::seconds(second_));
		if (t_)
			t_->async_wait(strand_.wrap(boost::bind(&Timer::on_timeout, this, boost::asio::placeholders::error)));
	}

	void reset(int second = -1)
	{
		if (second != -1)
			second_ = second;
		
		if (t_ && t_->expires_from_now(boost::chrono::seconds(second_)) > 0)
		{
			start();
		}
		else
		{
			// too late, timer has already expired!
			if (t_)
			{
				delete t_;
				t_ = NULL;
			}
			start();
		}
	}

	void stop()
	{
		if (t_)
		{
			t_->cancel();
			delete t_;
			t_ = NULL;
		}
	}

private:
	void on_timeout(const boost::system::error_code &e)
	{
		if (e != boost::asio::error::operation_aborted && handler_)
		{
			handler_();
		}
	}

	int second_;
	WaitHandler handler_;
	boost::asio::system_timer *t_;
	boost::asio::strand strand_;
};

NAMESPACE_EPILOG

#endif