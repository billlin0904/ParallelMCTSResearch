#pragma once

#include <memory>
#include <thread>
#include <atomic>
#include <unordered_map>

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
    int32_t session_id_;
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;
    boost::beast::flat_buffer buffer_;
    boost::circular_buffer<std::string> send_queue_;
    ServerCallback *callback_;

public:
    static const int32_t MAX_SEND_QUEUE_SIZE = 128;

    Session(int32_t session_id, boost::asio::ip::tcp::socket socket, ServerCallback* callback)
        : session_id_(session_id)
        , ws_(std::move(socket))
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
        ws_.set_option(boost::beast::websocket::stream_base::timeout::suggested(
                           boost::beast::role_type::server));
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
                    boost::beast::bind_front_handler(
                        &Session::OnRead,
                        shared_from_this()));
    }

    void Send(const std::string &message) {
        auto buffer(message);
        boost::beast::net::post(ws_.get_executor(),
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
                boost::beast::bind_front_handler(
                    &Session::OnWrite,
                    shared_from_this()));
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
};

class Listener : public std::enable_shared_from_this<Listener> {
    bool is_binray_;
    boost::asio::io_context& ioc_;
    boost::asio::ip::tcp::acceptor acceptor_;
    ServerCallback* callback_;
    mutable std::mutex mutex_;
    std::unordered_map<int32_t, std::shared_ptr<Session>> sessions_;
    std::string server_ver_;

public:
    Listener(const std::string &server_ver, boost::asio::io_context& ioc)
        : is_binray_(false)
        , ioc_(ioc)
        , acceptor_(ioc)
        , callback_(nullptr)
        , server_ver_(server_ver) {
    }

    Listener(const std::string& server_ver, boost::asio::io_context& ioc, boost::asio::ip::tcp::endpoint endpoint)
        : Listener(server_ver, ioc) {
        Bind(endpoint);
        Listen();
    }

    void Bind(boost::asio::ip::tcp::endpoint endpoint) {
        boost::system::error_code ec;
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            throw Exception(ec);
        }
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
        if (ec) {
            throw Exception(ec);
        }
        acceptor_.bind(endpoint, ec);
        if (ec) {
            throw Exception(ec);
        }
    }

    void Listen() {
        boost::system::error_code ec;
        acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
        if (ec) {
            throw Exception(ec);
        }
    }

    void Boardcast(const std::string& messag) {
        std::lock_guard<std::mutex> guard{ mutex_ };
        for (auto& sess : sessions_) {
            sess.second->Send(messag);
        }
    }

    void BoardcastExcept(const std::string &message, int32_t except_session_id) {
        std::lock_guard<std::mutex> guard{ mutex_ };
        for (auto& sess : sessions_) {
            if (sess.second->GetSessionID() != except_session_id) {
                sess.second->Send(message);
            }
        }
    }

    void SentTo(int32_t session_id, const std::string& message) {
        std::lock_guard<std::mutex> guard{ mutex_ };
        auto itr = sessions_.find(session_id);
        if (itr != sessions_.end()) {
            (*itr).second->Send(message);
        }
    }

    void Run(ServerCallback* callback) {
        callback_ = callback;
        if (!acceptor_.is_open()) {
            throw;
        }
        DoAccept();
    }

    void RemoveSession(int32_t session_id) {
        std::lock_guard<std::mutex> guard{ mutex_ };
        sessions_.erase(session_id);
    }

    void SetBinaryFormat(bool enable = true) {
        is_binray_ = enable;
    }
private:
    void DoAccept() {
        acceptor_.async_accept(
                    boost::beast::net::make_strand(ioc_),
                    boost::beast::bind_front_handler(
                        &Listener::OnAccept,
                        shared_from_this()));
    }

    void OnAccept(boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
        if (ec) {
            callback_->OnError(nullptr, Exception(ec));
        }
        auto id = NewSessionID();
        auto session = std::make_shared<Session>(id, std::move(socket), callback_);
        session->SetBinaryFormat(is_binray_);
        try {
            session->Start(server_ver_);
            std::lock_guard<std::mutex> guard{ mutex_ };
            sessions_.insert(std::make_pair(id, session));
        } catch (const std::exception&) {
        } catch (...) {
        }
        DoAccept();
    }
};

class WebSocketServer : public ServerCallback {
public:
    WebSocketServer(const std::string &server_ver, size_t max_thread = std::thread::hardware_concurrency())
        : max_thread_(max_thread)
        , ioc_(max_thread) {
        listener_ = std::make_shared<Listener>(server_ver, ioc_);
    }

    virtual ~WebSocketServer() {
        WaitAllThreadDone();
    }

    void Bind(boost::asio::ip::tcp::endpoint endpoint) {
        listener_->Bind(endpoint);
    }

    void Listen() {
        listener_->Listen();
    }

    void Bind(const std::string& addr, uint16_t port) {
        auto address = boost::beast::net::ip::make_address(addr);
        Bind(boost::asio::ip::tcp::endpoint{ address, port });
    }

    void Boardcast(const std::string& message) {
        listener_->Boardcast(message);
    }

    void SentTo(int32_t session_id, const std::string& message) {
        listener_->SentTo(session_id, message);
    }

    void BoardcastExcept(const std::string& message, int32_t except_session_id) {
        listener_->BoardcastExcept(message, except_session_id);
    }

    void Run() {
        boost::beast::net::signal_set signals(ioc_, SIGINT, SIGTERM);
        signals.async_wait([this](boost::system::error_code const&, int) {
            ioc_.stop();
        });

        worker_threads_.reserve(max_thread_);
        for (auto i = max_thread_ - 1; i > 0; --i) {
            worker_threads_.emplace_back([this] {
                ioc_.run();
            });
        }

        listener_->Run(this);
        ioc_.run();

        WaitAllThreadDone();
    }

    void RemoveSession(const std::shared_ptr<Session> &session) {
        listener_->RemoveSession(session->GetSessionID());
    }
private:
    void WaitAllThreadDone() {
        for (auto& thread : worker_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }
    size_t max_thread_;
    boost::beast::net::io_context ioc_;
    std::vector<std::thread> worker_threads_;
    std::shared_ptr<Listener> listener_;
};

}

