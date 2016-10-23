#ifndef __UNO_SESSION_H__
#define __UNO_SESSION_H__

#include "defines.h"
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/array.hpp>
#include <boost/signals2.hpp>

NAMESPACE_PROLOG
class Session;
typedef boost::asio::ip::tcp::endpoint endpoint;
typedef boost::shared_ptr<Session> SessionPtr;
typedef boost::function<void (std::string, SessionPtr)> ReadCallback;
class Session : public boost::enable_shared_from_this<Session>, private boost::noncopyable
{
public:
	explicit Session(boost::asio::io_service &io_service);
	virtual ~Session();
	boost::asio::ip::tcp::socket &socket() { return socket_; }
	void start();
	void send(const char *,size_t);
	void send(std::string);
	void close();
	void RegisterReadCallback(const ReadCallback &cb) { readcb_ = cb; }
	bool isAlive() { return socket_.is_open(); }
	static boost::signals2::signal<void (SessionPtr)> session_close;
#define subscribe(action,group,slot) session_ ## action.connect(group,slot);
private:
	void read();
	void handle_read(const boost::system::error_code &e, std::size_t bytes_transferred);
	void handle_write(const boost::system::error_code &e);
	boost::asio::io_service::strand strand_;
	boost::asio::ip::tcp::socket socket_;
	const static int max_length_ = 8192;
	boost::array<char, max_length_> data_;
	endpoint endpoint_;
	ReadCallback readcb_;
	bool disposed_;
};

NAMESPACE_EPILOG

#endif