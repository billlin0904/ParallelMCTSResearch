#pragma once

#include <memory>
#include <string>
#include <functional>
#include <cassert>
#include <queue>

#include "websocket.h"

#if ENABLE_SSL
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ssl/stream.hpp>
#endif

namespace websocket {

class WebSocketClient;

class WebSocketCallback {
public:
    virtual ~WebSocketCallback() = default;
    virtual void OnConnected(std::shared_ptr<WebSocketClient>) = 0;
    virtual void OnDisconnected(std::shared_ptr<WebSocketClient>) = 0;
    virtual void OnSend(std::shared_ptr<WebSocketClient>) = 0;
    virtual void OnReceive(std::shared_ptr<WebSocketClient>, const std::string &) = 0;
    virtual void OnError(std::shared_ptr<WebSocketClient>, const Exception &) = 0;
protected:
    WebSocketCallback() = default;
};

using tcp = boost::asio::ip::tcp;

class WebSocketClient : public std::enable_shared_from_this<WebSocketClient> {
protected:
    tcp::resolver resolver_;
    boost::asio::deadline_timer deadline_;
    std::atomic<bool> closing_;
    std::atomic<bool> connecting_;
    std::atomic<bool> timeouted_;
    std::string host_;
    std::string port_;
    boost::circular_buffer<std::string> send_queue_;
    std::unique_ptr<boost::beast::multi_buffer> buffer_;
    std::unique_ptr<WebSocketCallback> callback_;
    boost::asio::io_service::strand strand_;

    WebSocketClient(boost::asio::io_service& _ios, const std::string &_host, const std::string &_port, WebSocketCallback *callback);

    void OnConnecting(boost::system::error_code ec);

    void onResolve(boost::system::error_code ec, tcp::resolver::iterator result);

    void onDeadline();

    void OnHandshake(boost::system::error_code ec);

    void OnWrite(boost::system::error_code ec, std::size_t);

    void OnRead(boost::system::error_code ec, std::size_t);

    void onClosed(boost::beast::error_code);

    void DoWrite(const std::string & send_message);

    void OnSSLHandshake(boost::system::error_code ec);

    virtual void SetBinaryFormat(bool enable = true) = 0;
    virtual void SSLHandshake();
    virtual void DoHandshake() = 0;
    virtual void DoConnect(tcp::resolver::iterator result) = 0;
    virtual void Write() = 0;
#if ENABLE_SSL
    virtual void DoShutdown() = 0;
    virtual void ForceClose() = 0;
#endif
public:
    enum {
        TIME_INF = -1
    };

    static const int CONNECT_TIMEOUT;
    static const int MAX_SEND_QUEUE_SIZE;

    WebSocketClient(const WebSocketClient &) = delete;
    WebSocketClient& operator=(const WebSocketClient &) = delete;

    virtual ~WebSocketClient();

    static std::shared_ptr<WebSocketClient> MakeSocket(const std::string &scheme,
                                                       const std::string &host_,
                                                       const std::string &port_,
                                                       boost::asio::io_service& _ios,
                                                       WebSocketCallback* callback);

    void Connect();

    void SetTimeout(int timeout_seconds);

    void Send(const std::string &message);

    virtual void Receive() = 0;
    virtual void Disconnect() = 0;
#if ENABLE_SSL
    virtual int GetLocalPort() const = 0;
#endif
    bool IsConnecting() const {
        return connecting_;
    }
};

}
