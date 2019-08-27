#include "websocket_server.h"

namespace websocket {

int32_t NewSessionID() noexcept {
    static std::atomic<int32_t> sSessionID(10000);
    return ++sSessionID;
}

Listener::Listener(ServerCallback* callback, const std::string& server_ver, boost::asio::io_context& ioc)
	: is_binray_(false)
	, ioc_(ioc)
	, acceptor_(boost::asio::make_strand(ioc))
	, callback_(callback)
	, server_ver_(server_ver) {
}

Listener::Listener(ServerCallback* callback, const std::string& server_ver, boost::asio::io_context& ioc, boost::asio::ip::tcp::endpoint endpoint)
	: Listener(callback, server_ver, ioc) {
	Bind(endpoint);
	Listen();
}

void Listener::Bind(boost::asio::ip::tcp::endpoint endpoint) {
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

void Listener::Listen() {
	boost::system::error_code ec;
	acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
	if (ec) {
		throw Exception(ec);
	}
}

void Listener::Boardcast(const std::string& messag) {
	std::lock_guard<std::mutex> guard{ mutex_ };
	for (auto& sess : sessions_) {
		sess.second->Send(messag);
	}
}

void Listener::BoardcastExcept(const std::string& message, int32_t except_session_id) {
	std::lock_guard<std::mutex> guard{ mutex_ };
	for (auto& sess : sessions_) {
		if (sess.second->GetSessionID() != except_session_id) {
			sess.second->Send(message);
		}
	}
}

void Listener::SentTo(int32_t session_id, const std::string& message) {
	std::lock_guard<std::mutex> guard{ mutex_ };
	auto itr = sessions_.find(session_id);
	if (itr != sessions_.end()) {
		(*itr).second->Send(message);
	}
}

void Listener::Run() {
	if (!acceptor_.is_open()) {
		throw;
	}
	DoAccept();
}

void Listener::RemoveSession(int32_t session_id) {
	sessions_.erase(session_id);
}

void Listener::SetBinaryFormat(bool enable) {
	is_binray_ = enable;
}

void Listener::DoAccept() {
	acceptor_.async_accept(
		boost::beast::net::make_strand(ioc_),
		boost::beast::bind_front_handler(
			&Listener::OnAccept,
			shared_from_this()));
}

void Listener::OnAccept(boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
	if (ec) {
		callback_->OnError(nullptr, Exception(ec));
		return;
	}
	
	auto id = NewSessionID();	

	try {
		std::lock_guard<std::mutex> guard{ mutex_ };
		auto session = std::make_shared<Session>(id, ioc_, std::move(socket), callback_);
		session->SetBinaryFormat(is_binray_);

		session->Start(server_ver_);
		sessions_.insert(std::make_pair(id, session));
	}
	catch (const std::exception&) {
		return;
	}
	catch (...) {
		return;
	}
	DoAccept();
}

WebSocketServer::WebSocketServer(const std::string& server_ver, uint32_t max_thread)
	: max_thread_(max_thread)
	, ioc_(max_thread)
	, listener_(std::make_shared<Listener>(this, server_ver, ioc_)) {
}

WebSocketServer::~WebSocketServer() {
	WaitAllThreadDone();
}

void WebSocketServer::Bind(boost::asio::ip::tcp::endpoint endpoint) {
	listener_->Bind(endpoint);
}

void WebSocketServer::Listen() {
	listener_->Listen();
}

void WebSocketServer::Bind(const std::string& addr, uint16_t port) {
	auto address = boost::beast::net::ip::make_address(addr);
	Bind(boost::asio::ip::tcp::endpoint{ address, port });
}

void WebSocketServer::Boardcast(const std::string& message) {
	listener_->Boardcast(message);
}

void WebSocketServer::SentTo(int32_t session_id, const std::string& message) {
	listener_->SentTo(session_id, message);
}

void WebSocketServer::BoardcastExcept(const std::string& message, int32_t except_session_id) {
	listener_->BoardcastExcept(message, except_session_id);
}

void WebSocketServer::Run() {
	boost::beast::net::signal_set signals(ioc_, SIGINT, SIGTERM);
	signals.async_wait([this](boost::system::error_code const&, int) {
		ioc_.stop();
		});

	worker_threads_.reserve(max_thread_);
	for (uint32_t i = max_thread_ - 1; i > 0; --i) {
		worker_threads_.emplace_back([this] {
			ioc_.run();
			});
	}

	listener_->Run();
	ioc_.run();

	WaitAllThreadDone();
}

void WebSocketServer::RemoveSession(const std::shared_ptr<Session>& session) {
	listener_->RemoveSession(session->GetSessionID());
}

void WebSocketServer::WaitAllThreadDone() {
	for (auto& thread : worker_threads_) {
		if (thread.joinable()) {
			thread.join();
		}
	}
	worker_threads_.clear();
}

}
