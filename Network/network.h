#ifndef __UNO_NETWORK_H__
#define __UNO_NETWORK_H__
#include "defines.h"
#include "session.h"
#include "singleton.hpp"
#include "semaphore.hpp"
#include "../Common/common.h"

NAMESPACE_PROLOG
class NetCore 
{
public:
	virtual ~NetCore();
	virtual void run(std::string, int) = 0;
	virtual void send(std::string) = 0;
	virtual void broadcast(std::string) = 0;
	virtual void shutdown() = 0;
	virtual void join() = 0;
	boost::asio::io_service& io_service() { return io_service_; }
	ReadCallback callback_;
protected:
	NetCore();
	bool disposed_;
	boost::asio::io_service io_service_;
	boost::asio::signal_set *signals_;
};

SINGLETON_CLASS_BEGIN_1(Server, NetCore)
public:
	virtual ~Server();
private:
	Server();
	void run(std::string, int) override;
	void send(std::string) override;
	boost::asio::ip::tcp::acceptor *acceptor_;
	void handle_accept(SessionPtr session, const boost::system::error_code &ec);
	void handle_stop();
	void accept();
	void join();
	void broadcast(std::string);
	void shutdown();
	void onSessionClose(SessionPtr session);
	std::size_t thread_pool_size_;
	typedef std::list<SessionPtr> SessionList;
	SessionList sessions_;
	boost::shared_mutex session_mutex_;
	boost::shared_mutex map_mutex_;
	std::vector<boost::shared_ptr<boost::thread>> threads_;
SINGLETON_CLASS_END

SINGLETON_CLASS_BEGIN_1(Client, NetCore)
public:
	virtual ~Client();
	void run(std::string, int) override;
	void send(std::string) override;
	void connect(const std::string &ip, int port);
private:
	Client();
	void handle_connect(const boost::system::error_code &err);
	void broadcast(std::string);
	void shutdown();
	void join();
	SessionPtr session_;
	boost::shared_ptr<boost::thread> thread_;
SINGLETON_CLASS_END

class Network 
{
public:
	template<class T>
	static T *Run(int port, std::string ip = "")
	{
		core = T::instance();
		core->run(ip, port);
		return dynamic_cast<T *>(core);
	}

	static void Register(ReadCallback callback)
	{
		core->callback_ = callback;
	}

	static void Broadcast(const std::string &data)
	{
		core->broadcast(data);
	}	

	static void Send(const std::string &data)
	{
		core->send(data);
	}

	static void Shutdown()
	{
		core->shutdown();
	}

	static void Join()
	{
		core->join();
	}

	static boost::asio::io_service& io_service()
	{
		return core->io_service();
	}
private:
	static NetCore *core;
};

NAMESPACE_EPILOG
#endif