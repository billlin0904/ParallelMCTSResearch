#pragma once

#include <memory>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <parallel_hashmap/phmap.h>

#include <boost/system/error_code.hpp>
#include <boost/circular_buffer.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>

#include "websocket.h"

namespace websocket {

class Session;
class Exception;

class ServerCallback {
public:
    virtual ~ServerCallback() = default;
    virtual void OnConnected(std::shared_ptr<Session>) = 0;
    virtual void OnDisconnected(std::shared_ptr<Session>) = 0;
    virtual void OnSend(std::shared_ptr<Session>) = 0;
    virtual void OnReceive(std::shared_ptr<Session>, const std::string &) = 0;
    virtual void OnError(std::shared_ptr<Session>, const Exception&) = 0;
protected:
    ServerCallback() = default;
};

int32_t NewSessionID() noexcept;

class Session : public std::enable_shared_from_this<Session> {
public:
    static const int32_t MAX_SEND_QUEUE_SIZE = 128;

    Session(int32_t session_id, boost::asio::io_context& ioc, boost::asio::ip::tcp::socket socket, ServerCallback* callback)
        : session_id_(session_id)
        , ws_(std::move(socket))
        , strand_(boost::asio::make_strand(ioc))
        , send_queue_(MAX_SEND_QUEUE_SIZE)		
        , callback_(callback) {
        InitialStream(ws_);
    }

    virtual ~Session() {
    }

    void SetBinaryFormat(bool enable = true) {
        ws_.binary(enable);
    }

    bool IsBinaryFormat() const {
        return ws_.binary();
    }

    void Start(std::string server_ver) {
        ws_.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));
        ws_.set_option(boost::beast::websocket::stream_base::decorator(
                           [server_ver](boost::beast::websocket::response_type& res) {
                           res.set(boost::beast::http::field::server, server_ver);
                       }));
        ws_.async_accept(
                    boost::beast::bind_front_handler(
                        &Session::OnAccet,
                        shared_from_this()));
    }

    void Receive() {
		ws_.async_read(
			buffer_,
			boost::asio::bind_executor(strand_,
				std::bind(&Session::OnRead, shared_from_this(), std::placeholders::_1, std::placeholders::_2)));
    }

    void Send(const std::string &message) {
        auto buffer(message);
		boost::beast::net::post(strand_,
			boost::beast::bind_front_handler(
				&Session::DoWrite,
				shared_from_this(),
				buffer));
    }

	int32_t GetSessionID() const {
        return session_id_;
    }

private:
    void DoWrite(const std::string& send_message) {
        if (send_queue_.full()) {
            return;
        }

        auto write_in_progress = !send_queue_.empty();
        send_queue_.push_back(send_message);

        if (!write_in_progress) {
            Write();
        }
    }

    void Write() {
        ws_.async_write(
			boost::beast::net::buffer(&send_queue_.front()[0], send_queue_.front().size()),
			boost::asio::bind_executor(strand_,
				std::bind(&Session::OnWrite, shared_from_this(), std::placeholders::_1, std::placeholders::_2)
                ));
    }

    void OnWrite(boost::system::error_code ec, std::size_t) {
        if (ec) {
            callback_->OnError(shared_from_this(), Exception(ec));
            return;
        }

        callback_->OnSend(shared_from_this());

        if (!send_queue_.empty()) {
            send_queue_.pop_front();
        }

        if (!send_queue_.empty()) {
            Write();
        }
    }

    void OnAccet(boost::system::error_code ec) {
        if (ec) {
            callback_->OnError(shared_from_this(), Exception(ec));
            return;
        }
        callback_->OnConnected(shared_from_this());
    }

    void OnRead(boost::system::error_code ec, std::size_t) {
        if (ec == boost::beast::websocket::error::closed) {
            callback_->OnDisconnected(shared_from_this());
            return;
        }

        if (ec) {
            callback_->OnError(shared_from_this(), Exception(ec));
            return;
        }

        auto data = boost::beast::buffers_to_string(buffer_.data());
        buffer_.consume(data.size());
        callback_->OnReceive(shared_from_this(), data);
    }

	int32_t session_id_;	
	boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;
	boost::asio::strand<boost::asio::io_context::executor_type> strand_;
	boost::beast::flat_buffer buffer_;
	boost::circular_buffer<std::string> send_queue_;
	ServerCallback* callback_;
};

class Listener : public std::enable_shared_from_this<Listener> {
public:
	Listener(ServerCallback* callback, const std::string& server_ver, boost::asio::io_context& ioc);

	Listener(ServerCallback* callback, const std::string& server_ver, boost::asio::io_context& ioc, boost::asio::ip::tcp::endpoint endpoint);

	void Bind(boost::asio::ip::tcp::endpoint endpoint);

	void Listen();

	void Boardcast(const std::string& messag);

	void BoardcastExcept(const std::string& message, int32_t except_session_id);

	void SentTo(int32_t session_id, const std::string& message);

	void Run();

	void RemoveSession(int32_t session_id);

	void SetBinaryFormat(bool enable = true);
private:
	void DoAccept();

	void OnAccept(boost::system::error_code ec, boost::asio::ip::tcp::socket socket);

	bool is_binray_;
	boost::asio::io_context& ioc_;
	boost::asio::ip::tcp::acceptor acceptor_;
	std::mutex mutex_;
	ServerCallback* callback_;
	phmap::flat_hash_map<int32_t, std::shared_ptr<Session>> sessions_;
	std::string server_ver_;
};

class WebSocketServer : public ServerCallback {
public:
	WebSocketServer(const std::string& server_ver, uint32_t max_thread = std::thread::hardware_concurrency());

	virtual ~WebSocketServer();

	void Bind(boost::asio::ip::tcp::endpoint endpoint);

	void Listen();

	void Bind(const std::string& addr, uint16_t port);

	void Boardcast(const std::string& message);

	void SentTo(int32_t session_id, const std::string& message);

	void BoardcastExcept(const std::string& message, int32_t except_session_id);

	void Run();

	void RemoveSession(const std::shared_ptr<Session>& session);
private:
	void WaitAllThreadDone();

    uint32_t max_thread_;
    boost::beast::net::io_context ioc_;
    std::vector<std::thread> worker_threads_;
    std::shared_ptr<Listener> listener_;
};

}