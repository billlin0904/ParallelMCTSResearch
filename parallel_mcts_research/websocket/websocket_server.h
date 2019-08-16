#pragma once

#include <memory>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace websocket {

class Session;

struct SessionCallback {
    virtual void OnConnected(std::shared_ptr<Session>) = 0;
    virtual void OnDisconnected(std::shared_ptr<Session>) = 0;
    virtual void OnSend(std::shared_ptr<Session>) = 0;
    virtual void OnReceive(std::shared_ptr<Session>, const std::string &) = 0;
    virtual void OnError(std::shared_ptr<Session>, boost::system::error_code) = 0;
    virtual void OnTimeout(std::shared_ptr<Session>) = 0;
};

template<class NextLayer>
static void InitialStream(boost::beast::websocket::stream<NextLayer>& ws) {
    boost::beast::websocket::permessage_deflate pmd;
    pmd.client_enable = true;
    pmd.server_enable = true;
    pmd.compLevel = 3;
    ws.set_option(pmd);
    ws.auto_fragment(false);
    ws.read_message_max(64 * 1024 * 1024);
}

class Session : public std::enable_shared_from_this<Session> {
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;
    boost::beast::flat_buffer buffer_;
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    SessionCallback *callback_;
public:
    explicit Session(boost::asio::ip::tcp::socket socket)
        : ws_(std::move(socket))
        , strand_(ws_.get_executor())  {
        InitialStream(ws_);
    }

    void Run() {
        ws_.async_accept_ex([](boost::beast::websocket::response_type& res) {
            res.set(boost::beast::http::field::server, "Boost.Beast/" + std::to_string(BOOST_BEAST_VERSION) + "-Async");
        }, boost::asio::bind_executor(
                    strand_,
                    std::bind(
                        &Session::OnAccet,
                        shared_from_this(),
                        std::placeholders::_1)));
    }

    void Receive() {
        ws_.async_read(
                    buffer_,
                    boost::asio::bind_executor(
                        strand_,
                        std::bind(
                            &Session::OnRead,
                            shared_from_this(),
                            std::placeholders::_1,
                            std::placeholders::_2)));
    }

    void Send(const std::string &message) {
        auto buffer(message);

    }

private:
    void OnWrite(const std::string & send_message) {

    }

    void OnAccet(boost::system::error_code ec) {
        if (ec) {
            callback_->OnError(shared_from_this(), ec);
            return;
        }
        callback_->OnConnected(shared_from_this());
    }

    void OnRead(boost::system::error_code ec, std::size_t bytes_transferred) {
        auto data = boost::beast::buffers_to_string(buffer_.data());
        buffer_.consume(data.size());
        callback_->OnReceive(shared_from_this(), data);
    }
};

}

