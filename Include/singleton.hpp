#ifndef _UNO_SINGLETON_HPP_
#define _UNO_SINGLETON_HPP_
#include "defines.h"
#include <boost/shared_ptr.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/recursive_mutex.hpp>

NAMESPACE_PROLOG
template<typename T>
class Singleton
{
public:
	// **************************************************************************************
	// WARNING: Never call those function in destructor, it will result in undefined behavior.
	// **************************************************************************************
	static T *instance()
	{
		boost::lock_guard<boost::recursive_mutex> lock(mutex_);
		if (obj_.get() == nullptr)
			obj_.reset(new T());
		return obj_.get();
	}

	static boost::shared_ptr<T> smart_instance()
	{
		boost::lock_guard<boost::recursive_mutex> lock(mutex_);
		if (obj_.get() == nullptr)
			obj_.reset(new T());
		return obj_;
	}

private:
	static boost::shared_ptr<T> obj_;
	static boost::recursive_mutex mutex_;
};

template<typename T>
boost::shared_ptr<T> Singleton<T>::obj_;
template<typename T>
boost::recursive_mutex Singleton<T>::mutex_;

#define SINGLETON_MIDDLE(classname) { friend class Singleton<classname>;
#define SINGLETON_CLASS(classname) class classname : public Singleton<classname>
#define SINGLETON_CLASS_1(classname,base) class classname : public Singleton<classname>,public base
#define SINGLETON_CLASS_2(classname,base1,base2) class classname : public Singleton<classname>,public base1,public base2

// use this macro to define a singleton class
#define SINGLETON_CLASS_BEGIN(classname) SINGLETON_CLASS(classname) SINGLETON_MIDDLE(classname)
#define SINGLETON_CLASS_BEGIN_1(classname,base) SINGLETON_CLASS_1(classname,base) SINGLETON_MIDDLE(classname)
#define SINGLETON_CLASS_BEGIN_2(classname,base1,base2) SINGLETON_CLASS_2(classname,base1,base2) SINGLETON_MIDDLE(classname)
#define SINGLETON_CLASS_END };

NAMESPACE_EPILOG
#endif