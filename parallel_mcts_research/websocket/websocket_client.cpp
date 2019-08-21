#include "websocket_client.h"

namespace websocket {

namespace ssl = boost::asio::ssl;
namespace websocket = boost::beast::websocket;

const int WebSocketClient::CONNECT_TIMEOUT = 5;
const int WebSocketClient::MAX_SEND_QUEUE_SIZE = 128;

WebSocketClient::WebSocketClient(boost::asio::io_service& _ios, const std::string &_host, const std::string &_port, WebSocketCallback* callback)
    : resolver_(_ios)
    , deadline_(_ios)
    , host_(_host)
    , port_(_port)
    , buffer_(new boost::beast::multi_buffer())
    , send_queue_(MAX_SEND_QUEUE_SIZE)
    , strand_(_ios)
    , callback_(callback) {
}

WebSocketClient::~WebSocketClient() {
}

void WebSocketClient::SSLHandshake()  {
}

void WebSocketClient::Connect() {
    connecting_ = true;
    closing_ = false;
    timeouted_ = false;
#if ENABLE_SSL
    DoShutdown();
#endif
    send_queue_.clear();

    SetTimeout(CONNECT_TIMEOUT);

    resolver_.async_resolve({host_, port_},
                            std::bind(
                                &WebSocketClient::onResolve,
                                shared_from_this(),
                                std::placeholders::_1,
                                std::placeholders::_2));
}

void WebSocketClient::DoWrite(const std::string & send_message) {
    if (send_queue_.full()) {
        return;
    }

    auto write_in_progress = !send_queue_.empty();
    send_queue_.push_back(send_message);

    if (!write_in_progress) {
        Write();
    }
}

void WebSocketClient::Send(const std::string &message) {
    auto buffer(message);
    strand_.post(std::bind(&WebSocketClient::DoWrite, this, buffer));
}

void WebSocketClient::OnConnecting(boost::system::error_code ec)  {
    if (ec) {
        callback_->OnError(shared_from_this(), OperatorError::CONNECT_ERROR, ec);
        deadline_.cancel();
        return;
    }
    DoHandshake();
}

void WebSocketClient::OnHandshake(boost::system::error_code ec) {
    if (ec) {
        callback_->OnError(shared_from_this(), OperatorError::HANDSHARK_ERROR, ec);
        return;
    }
    connecting_ = false;
    timeouted_ = true;
    callback_->OnConnected(shared_from_this());
    SetTimeout(TIME_INF);
}

void WebSocketClient::OnWrite(boost::system::error_code ec, std::size_t) {
    if (ec) {
        callback_->OnError(shared_from_this(), OperatorError::WRITE_ERROR, ec);
        return;
    }
    if (!send_queue_.empty()) {
        send_queue_.pop_front();
    }
    if (!send_queue_.empty()) {
        Write();
    }
    callback_->OnSend(shared_from_this());
}

void WebSocketClient::OnRead(boost::system::error_code ec, std::size_t) {
    if (ec) {
        callback_->OnError(shared_from_this(), OperatorError::READ_ERROR, ec);
        return;
    }
    auto data = boost::beast::buffers_to_string(buffer_->data());
    buffer_->consume(data.size());
    callback_->OnReceive(shared_from_this(), data);
}

void WebSocketClient::onClosed(boost::beast::error_code) {
#if ENABLE_SSL
    ForceClose();
#endif
    connecting_ = false;
    callback_->OnDisconnected(shared_from_this());
    closing_ = false;
    send_queue_.clear();
}

void WebSocketClient::OnSSLHandshake(boost::system::error_code ec) {
    if (ec) {
        callback_->OnError(shared_from_this(), OperatorError::HANDSHARK_ERROR, ec);
        return;
    }
    SSLHandshake();
}

void WebSocketClient::SetTimeout(int timeout_seconds) {
    if (timeout_seconds == TIME_INF) {
        deadline_.expires_at(boost::posix_time::pos_infin);
    } else {
        deadline_.expires_from_now(boost::posix_time::seconds(timeout_seconds));
        timeouted_ = false;
        deadline_.async_wait(std::bind(&WebSocketClient::onDeadline, shared_from_this()));
    }
}

void WebSocketClient::onResolve(boost::system::error_code ec, tcp::resolver::iterator result) {
    if (ec) {
        callback_->OnError(shared_from_this(), OperatorError::CONNECT_ERROR, ec);
        return;
    }
    DoConnect(result);
}

void WebSocketClient::onDeadline() {
    if (closing_) {
        return;
    }
    if (deadline_.expires_at() <= boost::asio::deadline_timer::traits_type::now()) {
        deadline_.expires_at(boost::posix_time::pos_infin);
        Disconnect();
    }
    deadline_.async_wait(std::bind(&WebSocketClient::onDeadline, shared_from_this()));
}

class NoSSLWebSocket : public WebSocketClient {
    std::unique_ptr<websocket::stream<tcp::socket>> ws_;

protected:
    void DoHandshake() override {
        ws_->async_handshake(host_, "/",
                             std::bind(
                                 &NoSSLWebSocket::OnHandshake,
                                 shared_from_this(),
                                 std::placeholders::_1));
    }

    void DoConnect(tcp::resolver::iterator result) override {
        boost::asio::async_connect(
                    ws_->next_layer(),
                    result,
                    std::bind(
                        &NoSSLWebSocket::OnConnecting,
                        shared_from_this(),
                        std::placeholders::_1));
    }

    void Write() override {
        ws_->async_write(
                    boost::asio::buffer(&send_queue_.front()[0], send_queue_.front().size()),
                strand_.wrap(std::bind(
                                 &NoSSLWebSocket::OnWrite,
                                 shared_from_this(),
                                 std::placeholders::_1,
                                 std::placeholders::_2)));
    }
#if ENABLE_SSL
    void ForceClose() override {
        closing_ = true;
        boost::beast::error_code beast_ec;
        ws_->lowest_layer().cancel(beast_ec);
        //boost::system::error_code ec;
        //ws->close(websocket::close_code::normal, ec);
        //ws->lowest_layer().close();
    }

    void DoShutdown() override {
        boost::system::error_code ec;
        ws_->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        boost::beast::error_code beast_ec;
        ws_->lowest_layer().cancel(beast_ec);
        // Linux:
        // https://linux.die.net/man/3/close
        // All operations that are not canceled shall complete as if the close() blocked until the operations completed.
        //ws->close(websocket::close_code::normal, ec);
    }

    int GetLocalPort() const override {
        return ws_->lowest_layer().local_endpoint().port();
    }
#endif
public:
    explicit NoSSLWebSocket(boost::asio::io_service& _ios, const std::string &host, const std::string &port, WebSocketCallback* callback)
        : WebSocketClient(_ios, host, port, callback)
        , ws_(new websocket::stream<tcp::socket>(_ios)) {
        InitialStream(*ws_);
    }

    ~NoSSLWebSocket() override {
#if ENABLE_SSL
        ForceClose();
#endif
    }

    void SetBinaryFormat(bool enable = true) {
        ws_->binary(enable);
    }

    void Receive() override {
        ws_->async_read(
                    *buffer_,
                    std::bind(
                        &NoSSLWebSocket::OnRead,
                        shared_from_this(),
                        std::placeholders::_1,
                        std::placeholders::_2));
    }

    void Disconnect() override {
        SetTimeout(TIME_INF);
        closing_ = true;
        ws_->async_close(websocket::close_code::normal,
                         std::bind(&NoSSLWebSocket::onClosed,
                                   shared_from_this(),
                                   std::placeholders::_1));
    }
};

#if ENABLE_SSL
class SSLWebSocket : public WebSocket {
    ssl::context ssl_context_;
    std::unique_ptr<websocket::stream<ssl::stream<tcp::socket>>> ws_;

    void SSLHandshake() override {
        ws_->async_handshake(host_, "/",
                             std::bind(
                                 &SSLWebSocket::OnHandshake,
                                 shared_from_this(),
                                 std::placeholders::_1));
    }

protected:
    void DoHandshake() override {
        ws_->next_layer().async_handshake(
                    ssl::stream_base::client,
                    std::bind(
                        &SSLWebSocket::OnSSLHandshake,
                        shared_from_this(),
                        std::placeholders::_1));
    }

    void DoConnect(tcp::resolver::iterator result) override {
        boost::asio::async_connect(
                    ws_->next_layer().next_layer(),
                    result,
                    std::bind(
                        &SSLWebSocket::OnConnecting,
                        shared_from_this(),
                        std::placeholders::_1));
    }

    void Write() override {
        ws_->async_write(
                    boost::asio::buffer(&send_queue_.front()[0], send_queue_.front().size()),
                strand_.wrap(std::bind(
                                 &SSLWebSocket::OnWrite,
                                 shared_from_this(),
                                 std::placeholders::_1,
                                 std::placeholders::_2)));
    }

    void DoShutdown() override {
        boost::system::error_code ec;
        ws_->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

        boost::beast::error_code beast_ec;
        ws_->lowest_layer().cancel(beast_ec);
        // Linux:
        // https://linux.die.net/man/3/close
        // All operations that are not canceled shall complete as if the close() blocked until the operations completed.
        //ws->close(websocket::close_code::normal, ec);
    }

    void ForceClose() override {
        closing_ = true;
        boost::beast::error_code beast_ec;
        ws_->lowest_layer().cancel(beast_ec);
        // Linux:
        // https://linux.die.net/man/3/close
        // All operations that are not canceled shall complete as if the close() blocked until the operations completed.
        //ws->lowest_layer().close();
        //boost::system::error_code ec;
        //ws->close(websocket::close_code::normal, ec);
    }

    int GetLocalPort() const override {
        return ws_->lowest_layer().local_endpoint().port();
    }

public:
    explicit SSLWebSocket(boost::asio::io_service& _ios, const std::string &host, const std::string &port)
        : WebSocket(_ios, host, port)
        , ssl_context_(ssl::context::tlsv12_client)
        , ws_(new websocket::stream<ssl::stream<tcp::socket>>(_ios, ssl_context_)) {
        InitialStream(*ws_);
    }

    ~SSLWebSocket() override {
        ForceClose();
    }

    void SetBinaryFormat(bool enable = true) {
        ws_->binary(enable);
    }

    void Receive() override {
        ws_->async_read(
                    *buffer_,
                    std::bind(
                        &SSLWebSocket::OnRead,
                        shared_from_this(),
                        std::placeholders::_1,
                        std::placeholders::_2));
    }

    void Disconnect() override {
        SetTimeout(TIME_INF);
        closing_ = true;
        ws_->async_close(websocket::close_code::normal,
                         std::bind(&SSLWebSocket::onClosed,
                                   shared_from_this(),
                                   std::placeholders::_1));
    }
};
#endif
std::shared_ptr<WebSocketClient> WebSocketClient::MakeSocket(const std::string &scheme,
                                                             const std::string &host,
                                                             const std::string &port,
                                                             boost::asio::io_service& _ios,
                                                             WebSocketCallback* callback) {
#if ENABLE_SSL
    if (scheme == "wss") {
        return std::make_shared<SSLWebSocket>(_ios, host, port, callback);
    }
#endif
    return std::make_shared<NoSSLWebSocket>(_ios, host, port, callback);
}

}
