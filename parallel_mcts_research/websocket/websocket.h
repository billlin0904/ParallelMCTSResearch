#pragma once

#include <memory>
#include <string>
#include <functional>
#include <cassert>
#include <queue>

#include <boost/circular_buffer.hpp>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>

#if ENABLE_SSL
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ssl/stream.hpp>
#endif

namespace websocket {

class WebSocket;

enum OperatorError {
    RESOLVE_ERROR,
    HANDSHARK_ERROR,
    CONNECT_ERROR,
    READ_ERROR,
    WRITE_ERROR,
    TIMEOUT_ERROR,
};

struct WebSocketCallback {
    virtual void OnConnected(std::shared_ptr<WebSocket>) = 0;
    virtual void OnDisconnected(std::shared_ptr<WebSocket>) = 0;
    virtual void OnSend(std::shared_ptr<WebSocket>) = 0;
    virtual void OnReceive(std::shared_ptr<WebSocket>, const std::string &) = 0;
    virtual void OnError(std::shared_ptr<WebSocket>, OperatorError, boost::system::error_code) = 0;
};

using tcp = boost::asio::ip::tcp;

class WebSocket : public std::enable_shared_from_this<WebSocket> {
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

    WebSocket(boost::asio::io_service& _ios, const std::string &_host, const std::string &_port, WebSocketCallback *callback);

    void OnConnecting(boost::system::error_code ec);

    void onResolve(boost::system::error_code ec, tcp::resolver::iterator result);

    void onDeadline();

    void OnHandshake(boost::system::error_code ec);

    void OnWrite(boost::system::error_code ec, std::size_t);

    void OnRead(boost::system::error_code ec, std::size_t);

    void onClosed(boost::beast::error_code);

    void DoWrite(const std::string & send_message);

    void OnSSLHandshake(boost::system::error_code ec);

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

    WebSocket(const WebSocket &) = delete;
    WebSocket& operator=(const WebSocket &) = delete;

    virtual ~WebSocket();

    static std::shared_ptr<WebSocket> MakeSocket(const std::string &scheme,
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
