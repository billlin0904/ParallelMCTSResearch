#pragma once

#include <cstdint>
#include <atomic>
#include <string>
#include <sstream>

#include "gamestate.h"
#include "../utility.h"

namespace gomoku {

enum CommandID {
	ENTER_LOBBY = 1,
	ENTER_ROOM = 1000,
	START_ROUND,
	TURN,
};

static int32_t NewPacketID() noexcept {
	static std::atomic<int32_t> packet_id(10000);
	return ++packet_id;
}

static int32_t NewRoundID() noexcept {
	static std::atomic<int32_t> round_id(10000);
	return ++round_id;
}

static int32_t NewRoomID() noexcept {
	static std::atomic<int32_t> round_id(30000);
	return ++round_id;
}

static std::string GetJsonString(Document& document) {
	std::ostringstream ostr;
	OStreamWrapper wrapper{ ostr };
	PrettyWriter<OStreamWrapper> writer{ wrapper };
	document.Accept(writer);
	return ostr.str();
}

struct Encoder {
	static std::string EneterLobby(std::vector<int32_t>& available_room) {
		Document document;
		Value packet(kObjectType);
		packet.AddMember("cmd", CommandID::ENTER_LOBBY, document.GetAllocator());
		Value room_ids(kArrayType);
		for (auto room_id : available_room) {
			room_ids.PushBack(room_id, document.GetAllocator());
		}
		document.SetObject();
		document.AddMember("packet", packet, document.GetAllocator());
		document.AddMember("available_room", room_ids, document.GetAllocator());
		return GetJsonString(document);
	}

	static std::string EnterRoom(int32_t room_id, int32_t round_id) {
		Document document;
		Value packet(kObjectType);
		packet.AddMember("cmd", CommandID::ENTER_ROOM, document.GetAllocator());
		packet.AddMember("room_id", room_id, document.GetAllocator());
		packet.AddMember("round_id", round_id, document.GetAllocator());
		packet.AddMember("pid", NewPacketID(), document.GetAllocator());
		document.SetObject();
		document.AddMember("packet", packet, document.GetAllocator());
		return GetJsonString(document);
	}

	static std::string StartRound(int32_t room_id, int32_t round_id) {
		Document document;
		Value packet(kObjectType);
		packet.AddMember("cmd", CommandID::START_ROUND, document.GetAllocator());
		packet.AddMember("room_id", room_id, document.GetAllocator());
		packet.AddMember("round_id", round_id, document.GetAllocator());
		packet.AddMember("pid", NewPacketID(), document.GetAllocator());
		document.SetObject();
		document.AddMember("packet", packet, document.GetAllocator());
		return GetJsonString(document);
	}

	template <typename UCB1Policy>
	static std::string Turn(const GomokuGameMove& move,
		const GomokuGameState& state,
		const MCTS<GomokuGameState, GomokuGameMove, UCB1Policy>& mcts,
		int32_t room_id,
		int32_t round_id) {
		Document document;
		Value game_move(kObjectType);
		game_move.AddMember("row", move.row, document.GetAllocator());
		game_move.AddMember("column", move.column, document.GetAllocator());
		Value packet(kObjectType);
		packet.AddMember("cmd", CommandID::TURN, document.GetAllocator());
		packet.AddMember("room_id", room_id, document.GetAllocator());
		packet.AddMember("round_id", round_id, document.GetAllocator());
		packet.AddMember("player_id", state.GetPlayerID(), document.GetAllocator());
		packet.AddMember("pid", NewPacketID(), document.GetAllocator());
		packet.AddMember("move", game_move, document.GetAllocator());
		document.SetObject();
		document.AddMember("packet", packet, document.GetAllocator());
		//Serialize(mcts, document);
		return GetJsonString(document);
	}

	static std::string Turn(const GomokuGameMove& move,
		int32_t player_id,
		int32_t room_id,
		int32_t round_id) {
		Document document;
		Value game_move(kObjectType);
		game_move.AddMember("row", move.row, document.GetAllocator());
		game_move.AddMember("column", move.column, document.GetAllocator());
		Value packet(kObjectType);
		packet.AddMember("cmd", CommandID::TURN, document.GetAllocator());
		packet.AddMember("room_id", room_id, document.GetAllocator());
		packet.AddMember("round_id", round_id, document.GetAllocator());
		packet.AddMember("player_id", player_id, document.GetAllocator());
		packet.AddMember("pid", NewPacketID(), document.GetAllocator());
		packet.AddMember("move", game_move, document.GetAllocator());
		document.SetObject();
		document.AddMember("packet", packet, document.GetAllocator());
		return GetJsonString(document);
	}
};

}