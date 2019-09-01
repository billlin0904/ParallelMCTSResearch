#pragma once

#include <iostream>

#include "../mcts.h"
#include "../websocket/logger.h"
#include "../websocket/websocket_server.h"

#include "encoder.h"
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

	void EnterRoom(websocket::SessionID session_id, bool is_watch) {
		auto pid = NewPacketID();
		if (players_.empty() && !is_watch) {
			players_.insert(session_id);
			server_->SentTo(session_id, Encoder::EnterRoom(room_id_, round_id_));
		}
		else {
			watchers_.insert(session_id);
			BoardcastWatchers(Encoder::EnterRoom(room_id_, round_id_));
			logger_->debug("Server send EnterRoom pid: {}.", pid);
		}
	}

	void NewRound(websocket::SessionID session_id) {
		ai_.reset(new MCTS<GomokuGameState, GomokuGameMove>());
		state_.reset(new GomokuGameState());
		round_id_ = NewRoundID();

		auto msg = Encoder::StartRound(room_id_, round_id_);
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

		auto msg = Encoder::Turn(move, 0, room_id_, round_id_);
		BoardcastWatchers(msg);
	}

	int32_t GetRoundID() const {
		return round_id_;
	}

	void Turn(websocket::SessionID session_id, const GomokuGameMove& move) {
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
			Turn(session_id);
		}
		if (IsTerminal()) {
			++win_statis_[state_->GetWinner()];
			NewRound(session_id);
		}
	}

	int32_t GetPlayerCount() const {
		return static_cast<int32_t>(players_.size());
	}

	void LeavePlayer(websocket::SessionID session_id) {
		players_.erase(session_id);
		watchers_.erase(session_id);
	}

private:
	void Turn(websocket::SessionID session_id) {
#ifdef _DEBUG
		ai_->Initial(30, 30);
		auto move = ai_->ParallelSearch();
#else
		ai_->Initial(3000, 3000);
		auto move = ai_->ParallelSearch();
#endif        
		state_->ApplyMove(move);
		std::cout << "Server move: " << move.ToString() << std::endl << *state_;

		auto msg = Encoder::Turn(move, *state_, *ai_, room_id_, round_id_);
		logger_->debug("Server send Turn round: {} winrate: {}%.",
			round_id_, int32_t(ai_->GetCurrentNode()->GetWinRate() * 100));
		server_->SentTo(session_id, msg);
		BoardcastWatchers(msg);
	}

	bool IsTerminal() const {
		return state_->IsTerminal();
	}

	bool IsPlayerTurn(int32_t session_id) const {
		return players_.find(session_id) != players_.end();
	}

	void BoardcastWatchers(const std::string& message) {
		for (auto session_id : watchers_) {
			logger_->debug("Boardcast session id: {}.", session_id);
			server_->SentTo(session_id, message);
		}
	}
	int32_t room_id_;
	int32_t round_id_;
	int32_t round_count_;
	websocket::WebSocketServer* server_;
	phmap::flat_hash_map<int8_t, int32_t> win_statis_;
	phmap::flat_hash_set<websocket::SessionID> players_;
	phmap::flat_hash_set<websocket::SessionID> watchers_;
	std::unique_ptr<MCTS<GomokuGameState, GomokuGameMove>> ai_;
	std::unique_ptr<GomokuGameState> state_;
	std::shared_ptr<spdlog::logger> logger_;
};

class GomokuGameServer : public WebSocketServer {
public:
	GomokuGameServer()
		: WebSocketServer("Gomoku GameServer/1.0") {
		Logger::Get()
			.AddDebugOutputLogger()
			.AddFileLogger("gomoku.log");
		logger_ = websocket::Logger::Get().GetLogger("GomokuRoom");
		logger_->debug("Server is running ...");
	}

	void OnConnected(std::shared_ptr<websocket::Session> s) override {
		s->Receive();
		OnEnterLobby(s->GetSessionID());
		logger_->debug("Session id: {} connected!", s->GetSessionID());
	}

	void OnDisconnected(std::shared_ptr<websocket::Session> s) override {
		onClose(s);
	}

	void OnSend(std::shared_ptr<websocket::Session>) override {
	}

	void OnReceive(std::shared_ptr<websocket::Session> s, const std::string& str) override {
		OnRequest(s->GetSessionID(), str);
		s->Receive();
	}

	void OnError(std::shared_ptr<websocket::Session> s, const std::exception& e) override {
		logger_->debug("Session id:{} {}", s->GetSessionID(), e.what());
		onClose(s);
	}
private:
	void OnRequest(websocket::SessionID session_id, const std::string& str) {
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

	void Leave(int32_t room_id, websocket::SessionID session_id) {
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

	void onClose(const std::shared_ptr<websocket::Session>& s) {
		std::lock_guard<std::mutex> guard{ mutex_ };
		auto itr = session_room_.find(s->GetSessionID());
		if (itr != session_room_.end()) {
			Leave((*itr).second, s->GetSessionID());
		}
		logger_->debug("Session id: {} disconnected!", s->GetSessionID());
		RemoveSession(s);
	}

	void OnEnterLobby(websocket::SessionID session_id) {
		std::vector<int32_t> available_room;
		available_room.reserve(room_.size());
		for (auto& pair : room_) {
			available_room.push_back(pair.first);
		}
		SentTo(session_id, Encoder::EneterLobby(available_room));
	}

	void OnEnterRoom(const Value& packet,
		websocket::SessionID seesion_id,
		phmap::flat_hash_map<websocket::SessionID, int32_t>::iterator itr) {
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

	void EnterRoom(websocket::SessionID seesion_id, int32_t room_id, bool is_watch) {
		auto itr = room_.find(room_id);
		if (itr != room_.end()) {
			(*itr).second->EnterRoom(seesion_id, is_watch);
		}
	}

	void NewRoom(websocket::SessionID seesion_id, bool is_watch) {
		auto room_id = NewRoomID();
		session_room_.insert(std::make_pair(seesion_id, room_id));
		std::unique_ptr<GomokuRoom> room(new GomokuRoom(room_id, this));
		room->NewRound(seesion_id);
		room->EnterRoom(seesion_id, is_watch);
		room_.insert(std::make_pair(room_id, std::move(room)));
	}

	void OnTurn(const Value& packet, websocket::SessionID seesion_id, int32_t room_id) {
		auto itr = room_.find(room_id);
		if (itr == room_.end()) {
			return;
		}
		const GomokuGameMove move(packet["move"]["row"].GetInt(), packet["move"]["column"].GetInt());
		logger_->debug("Server receive client move: {} pid: {}.", move.ToString(), packet["pid"].GetInt());
		assert(packet["round_id"].GetInt() == (*itr).second->GetRoundID());
		(*itr).second->Turn(seesion_id, move);
	}

	std::mutex mutex_;
	phmap::flat_hash_map<websocket::SessionID, int32_t> session_room_;
	phmap::flat_hash_map<int32_t, std::unique_ptr<GomokuRoom>> room_;
	std::shared_ptr<spdlog::logger> logger_;
};

}