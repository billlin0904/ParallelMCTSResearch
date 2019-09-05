// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <iostream>

#include "../websocket/logger.h"
#include "../websocket/websocket_client.h"
#include "boost_coroutine.h"
#include "packetencoder.h"
#include "gamestate.h"

namespace gomoku {

using namespace websocket;

class GomokuGameClientCallback : public WebSocketCallback {
public:
	GomokuGameClientCallback()
		: room_id_(0)
		, round_id_(0)
		, logger_(Logger::Get().GetLogger("GomokuRoom")) {
	}

	void OnConnected(std::shared_ptr<WebSocketClient> s) override {
		state_.reset(new GomokuGameState());
		s->Send(PacketEncoder::EnterRoom(room_id_, round_id_));
		s->Receive();
	}

	void OnDisconnected(std::shared_ptr<WebSocketClient>) override {
	}

	void OnSend(std::shared_ptr<WebSocketClient>) override {
	}

	void OnReceive(std::shared_ptr<WebSocketClient> s, const std::string& str) override {
		Document document;
		document.Parse(str);

		if (document.HasParseError()) {
			s->Receive();
			return;
		}

		const auto& packet = document["packet"];
		auto cmd = static_cast<CommandID>(packet["cmd"].GetInt());
		logger_->debug("Client receive cmd: {} pid: {}.", cmd, packet["pid"].GetInt());

		std::lock_guard<std::mutex> guard{ mutex_ };
		if (cmd == CommandID::ENTER_ROOM || cmd == CommandID::START_ROUND) {
			if (cmd == CommandID::START_ROUND) {
				round_id_ = packet["round_id"].GetInt();
			}
			room_id_ = packet["room_id"].GetInt();
			EnterRoomAsync(s, cmd);
		}
		else if (cmd == CommandID::TURN) {
			TurnAsync(s, packet);
		}
		s->Receive();
	}

	void OnError(std::shared_ptr<WebSocketClient>, const Exception& e) override {
		std::cerr << e.what() << std::endl;
		std::cerr.flush();
	}
private:
	boost::future<void> EnterRoomAsync(const std::shared_ptr<WebSocketClient>& s, CommandID cmd) {
		if (cmd == CommandID::START_ROUND) {
			if (round_id_ > 0 && !state_->IsEmptyMove()) {
				assert(state_->IsTerminal());
				state_.reset(new GomokuGameState());
				ai_.reset(new MCTS<GomokuGameState, GomokuGameMove>());
#ifdef _DEBUG        
				ai_->Initial(10, 30);
#else
				ai_->Initial(100, 2000);				
#endif
				auto move = await ai_->ParallelSearchAsync();
				state_->ApplyMove(move);
				s->Send(PacketEncoder::Turn(move, *state_, *ai_, room_id_, round_id_));
				logger_->debug("Client send Turn round_id: {}", round_id_);
			}
		}
		else if (cmd == CommandID::ENTER_ROOM) {
			state_.reset(new GomokuGameState());
			ai_.reset(new MCTS<GomokuGameState, GomokuGameMove>());
			auto move = co_await ai_->ParallelSearchAsync();
			state_->ApplyMove(move);
			s->Send(PacketEncoder::Turn(move, *state_, *ai_, room_id_, round_id_));
			logger_->debug("Client send Turn round_id: {}", round_id_);
		}
		co_return;
	}

	boost::future<void> TurnAsync(const std::shared_ptr<WebSocketClient>& s, const Value& packet) {
		assert(packet["round_id"].GetInt() == round_id_);

		GomokuGameMove move(packet["move"]["row"].GetInt(), packet["move"]["column"].GetInt());
		assert(state_->IsLegalMove(move));

		state_->ApplyMove(move);
		ai_->SetOpponentMove(move);

		if (state_->IsTerminal()) {
			logger_->debug("Client receive final move round id:{}, Wait new round.", round_id_);
			co_return;
		}

		auto search_move = co_await ai_->ParallelSearchAsync();
		assert(state_->IsLegalMove(search_move));
		state_->ApplyMove(search_move);
		s->Send(PacketEncoder::Turn(search_move, *state_, *ai_, room_id_, round_id_));
		logger_->debug("Client send Turn round_id: {}", round_id_);
		std::cout << "Client move: " << move.ToString() << std::endl << *state_;
		co_return;
	}

	int32_t room_id_;
	int32_t round_id_;
	std::mutex mutex_;
	std::unique_ptr<MCTS<GomokuGameState, GomokuGameMove>> ai_;
	std::unique_ptr<GomokuGameState> state_;
	std::shared_ptr<spdlog::logger> logger_;
};

}