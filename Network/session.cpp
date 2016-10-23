#include "session.h"
#include "log.hpp"
#include <boost/bind.hpp>
#include <iostream>

USING_NAMESPACE
using namespace boost::asio;
using namespace boost;

boost::signals2::signal<void (SessionPtr)> Session::session_close;
void Session::start()
{
	endpoint_ = socket_.remote_endpoint();
	LOG(info) << "[" << (endpoint_.address().is_v4() ? "ipv4" : "ipv6") << "]" << 
		endpoint_.address() << ":" << endpoint_.port() << " connected.";
	read();
}

void Session::read()
{
	socket_.async_read_some(buffer(data_),strand_.wrap(
		bind(&Session::handle_read, shared_from_this(),
		placeholders::error, placeholders::bytes_transferred)));
}

void Session::handle_read(const boost::system::error_code &error, size_t bytes_transferred)
{
	if (disposed_)
		return;

	if (!error)
	{
		if (readcb_)
		{
			std::string str(data_.data(), bytes_transferred);
			readcb_(str, shared_from_this());
			data_.assign(0);
		}
		read();
	}
	else if (error == asio::error::eof)
	{
		LOG(warning) << endpoint_.address().to_string() << ":" << endpoint_.port() << " disconnected.";
		close();
	}
	else
	{
		LOG(error) << error.value() << " - " << error.message();
		if (error.value() == 10054)
			close();
	}
}

void Session::close()
{
	disposed_ = true;
	socket_.shutdown(socket_base::shutdown_both);
	socket_.close();
	session_close(shared_from_this());
}

Session::Session(io_service &io_service) : strand_(io_service), socket_(io_service), disposed_(false)
{
	data_.assign(0);
}

Session::~Session()
{
	std::cout << "[for debug] session destroyed" << std::endl;
}

void Session::send(std::string data)
{
	send(data.c_str(), data.size());
}

void Session::handle_write(const boost::system::error_code &err)
{
	if (disposed_)
		return;
	if (err)
	{
		LOG(error) << "write error: " << err.message();
	}
}

void Session::send(const char *data, size_t size)
{
	socket_.async_write_some(buffer(data, size),strand_.wrap(
		bind(&Session::handle_write, shared_from_this(), placeholders::error)));
}