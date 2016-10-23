#include "network.h"
#include "log.hpp"

USING_NAMESPACE
using namespace boost;
using namespace boost::asio;
using namespace boost::asio::ip;
using namespace boost::system;

const int thread_pool_size = 4;

NetCore *Network::core = NULL;

NetCore::~NetCore() 
{
	io_service_.stop();
	if (signals_)
	{
		delete signals_;
		signals_ = NULL;
	}
}

NetCore::NetCore() : disposed_(false), signals_(NULL)
{
	signals_ = new boost::asio::signal_set(io_service_, SIGINT, SIGTERM);
}

void Server::accept()
{
	SessionPtr new_session(new Session(io_service_));
	acceptor_->async_accept(new_session->socket(),
		boost::bind(&Server::handle_accept, this, new_session, placeholders::error));
}

Server::Server() : thread_pool_size_(thread_pool_size),acceptor_(NULL)
{
	Session::subscribe(close,0,boost::bind(&Server::onSessionClose, this, _1));
	signals_->async_wait(boost::bind(&Server::handle_stop, this));
}

void Server::handle_stop()
{
	std::cout << "[debug] handle_stop called." << std::endl;
	acceptor_->cancel();
	SessionList::iterator iter;
	SessionList temp_list = sessions_;
	for (iter = temp_list.begin(); iter != temp_list.end(); ++iter)
	{
		(*iter)->close();
	}
}

void Server::handle_accept(SessionPtr session, const system::error_code &err)
{
	if (disposed_)
		return;

	if (!err)
	{
		session->start();
		writelock lock(session_mutex_);
		sessions_.push_back(session);
		lock.unlock();
		if (callback_)
			session->RegisterReadCallback(callback_);
		accept();
	}
}

void Server::onSessionClose(SessionPtr session)
{
	LOG(debug) << "server session close";
	SessionList::iterator iter;
	writelock lock(session_mutex_);
	for (iter = sessions_.begin(); iter != sessions_.end(); ++iter)
	{
		if ((*iter) == session)
		{
			sessions_.erase(iter);
			break;
		}
	}
}

Server::~Server()
{
	if (acceptor_)
		delete acceptor_;
}

void Server::run(std::string ip_addr, int port)
{
	endpoint ep;
	if (ip_addr.length() != 0)
	{
		address_v4 addr = address_v4::from_string(ip_addr);
		ep.address(ip::address(addr));
		ep.port(port);
	}
	else
	{
		ep = endpoint(tcp::v4(), port);
	}
	acceptor_ = new tcp::acceptor(io_service_, ep);
	accept();
	LOG(info) << "Server started, Listening at port: " << ep.port();
	for (size_t i = 0; i < thread_pool_size_; ++i)
	{
		shared_ptr<boost::thread> thread(new boost::thread(bind(&io_service::run, &io_service_)));
		threads_.push_back(thread);
	}
}

void Server::join()
{
	threads_[0]->join();
}

void Client::join()
{
	thread_->join();
}

void Server::send(std::string)
{
	LOG(warning) << "server never use send, use broadcast instead.";
}

Client::Client()
{
	signals_->async_wait(boost::bind(&Client::handle_stop, this));
}

void Client::handle_stop()
{
	session_->close();
}

Client::~Client()
{

}

void Client::run(std::string ip_addr, int port)
{
	connect(ip_addr, port);
	thread_.reset(new boost::thread(bind(&io_service::run, &io_service_)));
}

void Client::handle_connect(const error_code &err)
{
	if (disposed_)
		return;

	if (!err)
	{
		session_->start();
		if (callback_)
			session_->RegisterReadCallback(callback_);
	}
	else
		LOG(error) << "connect error : " << err.message();
}

void Client::connect(const std::string &ip_addr, const int port)
{
	address_v4 addr = address_v4::from_string(ip_addr);
	endpoint ep(ip::address(addr), port);
	session_.reset(new Session(io_service_));
	tcp::socket &sock = session_->socket();
	sock.async_connect(ep, boost::bind(&Client::handle_connect, this, placeholders::error));
	LOG(info) << "connecting to " << ep.address() << ":" << ep.port() << "..." ;
}

void Server::broadcast(std::string data)
{
	// broadcast
	SessionList::iterator iter;
	readlock lock(session_mutex_);
	for (iter = sessions_.begin(); iter != sessions_.end(); ++iter)
	{
		(*iter)->send(data);
	}
}

void Client::broadcast(std::string data)
{
	LOG(warning) << "client never use broadcast.";
}

void Client::send(std::string data)	
{
	session_->send(data);
}

void Server::shutdown()
{
	disposed_ = true;
	SessionList::iterator iter;
	readlock lock(session_mutex_);
	for (iter = sessions_.begin(); iter != sessions_.end(); ++iter)
	{
		(*iter)->close();
	}
}

void Client::shutdown()
{
	disposed_ = true;
	session_->close();
}
