#ifndef _UNO_SEMAPHORE_HPP_
#define _UNO_SEMAPHORE_HPP_
#include "defines.h"
#include <boost/thread.hpp>

NAMESPACE_PROLOG
class Semaphore
{
	unsigned int count_;
	boost::mutex mutex_;
	boost::condition_variable condition_;
public:
	explicit Semaphore(unsigned int initial) : count_(initial){}
	void signal()
	{
		{
			boost::lock_guard<boost::mutex> lock(mutex_);
			++count_;
		}

		condition_.notify_one();
	}

	void wait()
	{
		boost::unique_lock<boost::mutex> lock(mutex_);
		while (count_ == 0)
		{
			condition_.wait(lock);
		}
		--count_;
	}

};
NAMESPACE_EPILOG
#endif