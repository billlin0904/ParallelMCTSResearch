#pragma once

#include <string>
#include <exception>

#include <boost/circular_buffer.hpp>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>

namespace websocket {

template <class NextLayer>
static void InitialStream(boost::beast::websocket::stream<NextLayer>& ws) {
    boost::beast::websocket::permessage_deflate pmd;
    pmd.client_enable = true;
    pmd.server_enable = true;
    pmd.compLevel = 3;
    ws.set_option(pmd);
    ws.auto_fragment(false);
    ws.read_message_max(64 * 1024 * 1024);
}

class Exception : public std::exception {
public:
	explicit Exception(boost::system::error_code ec);

	virtual ~Exception() = default;

	const char* what() const noexcept override;
private:
	std::string message_;
};

}
