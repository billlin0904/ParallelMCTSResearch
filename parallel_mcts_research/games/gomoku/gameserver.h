// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <iostream>

#include "../mcts.h"
#include "../websocket/logger.h"
#include "../websocket/websocket_server.h"

#include "boost_coroutine.h"
#include "packetencoder.h"
#include "gamestate.h"

namespace gomoku {

using namespace websocket;

class GomokuRoom {
public:
	GomokuRoom(int32_t room_id, WebSocketServer* server)
		: room_id_(room_id)
		, round_id_(NewRoundID())
		, round_count_(1)
		, server_(server)
		, logger_(Logger::Get().GetLogger("GomokuRoom")) {
		win_statis_[PLAYER1] = 0;
		win_statis_[PLAYER2] = 0;
	}

	void EnterRoom(SessionID session_id, bool is_watch) {
		if (players_.empty() && !is_watch) {
			players_.insert(session_id);
			server_->SentTo(session_id, PacketEncoder::EnterRoom(room_id_, round_id_));
		}
		else {
			watchers_.insert(session_id);
			BoardcastWatchers(PacketEncoder::EnterRoom(room_id_, round_id_));
			logger_->debug("Server send EnterRoom");
		}
	}

	void NewRound(SessionID session_id) {
		ai_.reset(new MCTS<GomokuGameState, GomokuGameMove>());
		state_.reset(new GomokuGameState());
		round_id_ = NewRoundID();
#ifdef _DEBUG
		ai_->Initial(300, 300);
#else
		ai_->Initial(3000, 3000);
#endif
		const auto msg = PacketEncoder::StartRound(room_id_, round_id_);
		server_->SentTo(session_id, msg);
		BoardcastWatchers(msg);

		logger_->debug("Round:{} O Win: {}({}%), @¡@Win: {}({}%)",
			round_count_,
			win_statis_[PLAYER1], win_statis_[PLAYER1] / double(round_count_),
			win_statis_[PLAYER2], win_statis_[PLAYER2] / double(round_count_));
		logger_->debug("Server send NewRound round id:{}.", round_id_);

		++round_count_;
	}

	void SetOpponentMove(const GomokuGameMove& move) {
		assert(state_->IsLegalMove(move));

		state_->ApplyMove(move);
		ai_->SetOpponentMove(move);

		BoardcastWatchers(PacketEncoder::Turn(move, 0, room_id_, round_id_));
	}

	int32_t GetRoundID() const {
		return round_id_;
	}

	boost::future<void> TurnAsync(SessionID session_id, const GomokuGameMove& move) {
		assert(state_->IsLegalMove(move));
		if (!IsPlayerTurn(session_id)) {
			return;
		}
		if (IsTerminal()) {
			++win_statis_[state_->GetWinner()];
			NewRound(session_id);
			return;
		}
		SetOpponentMove(move);
		if (!IsTerminal()) {
			co_await TurnAsync(session_id);
		}
		if (IsTerminal()) {
			++win_statis_[state_->GetWinner()];
			NewRound(session_id);
		}
	}

	int32_t GetPlayerCount() const {
		return static_cast<int32_t>(players_.size());
	}

	void LeavePlayer(SessionID session_id) {
		ai_->Cancel();
		players_.erase(session_id);
		watchers_.erase(session_id);
	}

private:
	boost::future<void> TurnAsync(SessionID session_id) {
		auto move = co_await ai_->ParallelSearchAsync();
  
		state_->ApplyMove(move);
		std::cout << "Server move: " << move.ToString() << std::endl << *state_;

		auto msg = PacketEncoder::Turn(move, *state_, *ai_, room_id_, round_id_);
		logger_->debug("Server send Turn round: {} winrate: {}%.",
			round_id_, 
			int32_t(ai_->GetCurrentNode()->GetWinRate() * 100));
		server_->SentTo(session_id, msg);
		BoardcastWatchers(msg);		
	}

	bool IsTerminal() const {
		return state_->IsTerminal();
	}

	bool IsPlayerTurn(SessionID session_id) const {
		return players_.find(session_id) != players_.end();
	}

	void BoardcastWatchers(const std::string& message) {
		server_->Boardcast(watchers_, message);
	}
	int32_t room_id_;
	int32_t round_id_;
	int32_t round_count_;
	WebSocketServer* server_;
	phmap::flat_hash_map<int8_t, int32_t> win_statis_;
	SessionSet players_;
	SessionSet watchers_;
	std::unique_ptr<MCTS<GomokuGameState, GomokuGameMove>> ai_;
	std::unique_ptr<GomokuGameState> state_;
	std::shared_ptr<spdlog::logger> logger_;
};

class GomokuGameServer : public WebSocketServer {
public:
	using SessionHashMap = phmap::flat_hash_map<SessionID, int32_t>;
	using RoomHashMap = phmap::flat_hash_map<int32_t, std::unique_ptr<GomokuRoom>>;

	GomokuGameServer()
		: WebSocketServer("Gomoku GameServer/1.0") {
		Logger::Get()
			.AddDebugOutputLogger()
			.AddFileLogger("gomoku.log");
		logger_ = websocket::Logger::Get().GetLogger("GomokuRoom");
		logger_->debug("Server is running ...");
	}

	void OnConnected(std::shared_ptr<Session> s) override {
		s->Receive();
		OnEnterLobby(s->GetSessionID());
		logger_->debug("Session id: {} connected!", s->GetSessionID());
	}

	void OnDisconnected(std::shared_ptr<Session> s) override {
		onClose(s);
	}

	void OnSend(std::shared_ptr<Session>) override {
	}

	void OnReceive(std::shared_ptr<Session> s, const std::string& str) override {
		OnRequest(s->GetSessionID(), str);
		s->Receive();
	}

	void OnError(std::shared_ptr<Session> s, const std::exception& e) override {
		logger_->debug("Session id:{} {}", s->GetSessionID(), e.what());
		onClose(s);
	}
private:
	void OnRequest(SessionID session_id, const std::string& str) {
		Document document;
		document.Parse(str);

		if (document.HasParseError()) {
			return;
		}

		const auto& packet = document["packet"];
		auto cmd = static_cast<CommandID>(packet["cmd"].GetInt());

		std::lock_guard<std::mutex> guard{ mutex_ };
		auto itr = session_room_.find(session_id);

		switch (cmd) {
		case CommandID::ENTER_LOBBY:
			OnEnterLobby((*itr).first);
			break;
		case CommandID::ENTER_ROOM:
			OnEnterRoom(packet, session_id, itr);
			break;
		case CommandID::TURN:
			OnTurn(packet, session_id, (*itr).second);
			break;
		case CommandID::START_ROUND:
		default:
			break;
		}
	}

	void Leave(int32_t room_id, SessionID session_id) {
		auto itr = room_.find(room_id);
		if (itr == room_.end()) {
			return;
		}

		(*itr).second->LeavePlayer(session_id);
		if ((*itr).second->GetPlayerCount() == 0) {
			room_.erase(room_id);
			session_room_.erase(session_id);
		}
	}

	void onClose(const std::shared_ptr<Session>& s) {
		std::lock_guard<std::mutex> guard{ mutex_ };
		auto itr = session_room_.find(s->GetSessionID());
		if (itr != session_room_.end()) {
			Leave((*itr).second, s->GetSessionID());
		}
		logger_->debug("Session id: {} disconnected!", s->GetSessionID());
		RemoveSession(s);
	}

	void OnEnterLobby(SessionID session_id) {
		std::vector<int32_t> available_room;
		available_room.reserve(room_.size());
		for (auto& pair : room_) {
			available_room.push_back(pair.first);
		}
		SentTo(session_id, PacketEncoder::EneterLobby(available_room));
	}

	void OnEnterRoom(const Value& packet,
		SessionID seesion_id,
		SessionHashMap::iterator itr) {
		bool is_watch = false;

		if (packet.HasMember("is_watch")) {
			is_watch = packet["is_watch"].GetBool();
		}

		if (is_watch) {
			if (!session_room_.empty()) {
				auto itr = session_room_.begin();
				EnterRoom(seesion_id, (*itr).second, is_watch);
			}
		}
		else {
			if (itr == session_room_.end()) {
				NewRoom(seesion_id, is_watch);
			}
			else {
				EnterRoom(seesion_id, (*itr).second, is_watch);
			}
		}
	}

	void EnterRoom(SessionID seesion_id, int32_t room_id, bool is_watch) {
		auto itr = room_.find(room_id);
		if (itr != room_.end()) {
			(*itr).second->EnterRoom(seesion_id, is_watch);
		}
	}

	void NewRoom(SessionID seesion_id, bool is_watch) {
		auto room_id = NewRoomID();
		session_room_.insert(std::make_pair(seesion_id, room_id));
		std::unique_ptr<GomokuRoom> room(new GomokuRoom(room_id, this));
		room->NewRound(seesion_id);
		room->EnterRoom(seesion_id, is_watch);
		room_.insert(std::make_pair(room_id, std::move(room)));
	}

	boost::future<void> OnTurn(const Value& packet, SessionID seesion_id, int32_t room_id) {
		const GomokuGameMove move(packet["move"]["row"].GetInt(), packet["move"]["column"].GetInt());
		logger_->debug("Server receive client move: {}.", move.ToString());
		
		auto itr = room_.find(room_id);
		if (itr == room_.end()) {
			co_return;
		}
		co_await (*itr).second->TurnAsync(seesion_id, move);
		logger_->debug("Server final thinking!");
	}

	std::mutex mutex_;
	SessionHashMap session_room_;
	RoomHashMap room_;
	std::shared_ptr<spdlog::logger> logger_;
};

}