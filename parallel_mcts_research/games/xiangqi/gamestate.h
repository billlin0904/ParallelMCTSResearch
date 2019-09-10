// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <cassert>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <optional>

#include <tweakme.h>
#include <rng.h>

#include "pieces.h"
#include "gamemove.h"

namespace xiangqi {

using namespace mcts;

static const int32_t MIN_ROW = 1;
static const int32_t MIN_COL = 1;

static const int32_t MAX_ROW = 10;
static const int32_t MAX_COL = 9;

using BoardStates = HashMap<XiangQiGameMove, Pieces>;

template <typename C, typename Function>
C Filter(const C& container, Function&& f) {
	C filtered(container);
	filtered.erase(std::remove_if(filtered.begin(), filtered.end(), f), filtered.end());
	return filtered;
}

inline std::ostream& operator<<(std::ostream& ostr, const BoardStates& board_states) {
	for (auto row = MIN_ROW; row <= MAX_ROW; ++row) {
		for (auto col = MIN_COL; col <= MAX_COL; ++col) {
			XiangQiGameMove move(row, col);
			auto itr = board_states.find(move);
			if (itr != board_states.end()) {
				std::ostringstream ss;
				ss << (*itr).second.color << (*itr).second.type;
				ostr << std::setw(2) << ss.str() << "|";
			}
			else {
				ostr << std::setw(2) << "   " << "|";
			}
		}
		ostr << "\n";
	}
	ostr << "\n----------\n";
	return ostr;
}


const std::vector<Pieces>& GetRedPieces();

const std::vector<Pieces>& GetBlackPieces();

struct Rules {
	static std::vector<XiangQiGameMove> MovesOnSameLine(Colors color, int8_t row, int8_t col, const BoardStates& board);

	static std::vector<XiangQiGameMove> GetCheLegalMove(Colors color, int8_t row, int8_t col, const BoardStates& board);

	static std::vector<XiangQiGameMove> GetMaLegalMove(Colors color, int8_t row, int8_t col, const BoardStates& board);

	static std::vector<XiangQiGameMove> GetPaoLegalMove(Colors color, int8_t row, int8_t col, const BoardStates& board);

	static std::vector<XiangQiGameMove> GetShiLegalMove(Colors color, int8_t row, int8_t col, const BoardStates& board);

	static std::vector<XiangQiGameMove> GetBingLegalMove(Colors color, int8_t row, int8_t col, const BoardStates& board);

	static std::vector<XiangQiGameMove> GetXiangLegalMove(Colors color, int8_t row, int8_t col, const BoardStates& board);

	static std::vector<XiangQiGameMove> GetJiangLegalMove(Colors color, int8_t row, int8_t col, const BoardStates& board);

	static std::vector<XiangQiGameMove> GetPossibleMoves(const Pieces& pieces, const BoardStates& board);
};

class XiangQiGameAgent {
public:
	explicit XiangQiGameAgent(int8_t player_id)
		: player_id_(player_id) {
		if (player_id_ == 1) {
			pieces_ = GetRedPieces();
		}
		else {
			pieces_ = GetBlackPieces();
		}
	}

	void Initial(BoardStates& board_states) {
		for (auto pieces : pieces_) {
			board_states.insert(std::make_pair(pieces.pos, pieces));
		}
	}

	Colors GetColor() const {
		return player_id_ != 1 ? COLOR_BLACK : COLOR_RED;
	}

	const HashSet<Pieces>& GetLegalMoves() const {
		return legal_moves_;
	}

	bool IsLegalMove(const Pieces& move) const {
		return legal_moves_.find(move) != legal_moves_.end();
	}

	Pieces GetRandomMove() const {
		assert(!legal_moves_.empty());
		auto itr = std::next(std::begin(legal_moves_),
			RNG::Get()(0, int32_t(legal_moves_.size() - 1)));
		return *itr;
	}

	void SetPiece(const Pieces& pieces) {
		assert(IsLegalMove(pieces));
		auto itr = std::find_if(pieces_.begin(), pieces_.end(),
			[pieces](const Pieces& other) {
				return pieces.type == other.type;
			});
		assert(itr != pieces_.end());
		(*itr).pos = pieces.pos;
	}

	void ClearBoardStates(BoardStates& board_states) {
		for (auto pieces : pieces_) {
			auto itr = board_states.find(pieces.pos);
			if (itr == board_states.end()) {
				continue;
			}
			assert(pieces.color == GetColor());
			//assert((*itr).second.color == GetColor());
			board_states.erase(pieces.pos);
		}
	}

	void RemovePieces(const Pieces& pieces, BoardStates& board_states) {
		ClearBoardStates(board_states);
		auto itr = std::find_if(pieces_.begin(), pieces_.end(), [&pieces](const Pieces& other) {
			return pieces.pos == other.pos;
			});
		assert(itr != pieces_.end());
		//std::cout << pieces << " capture " << (*itr) << "\n";		
		pieces_.erase(itr);
		UpdateBoardState(board_states);
	}

	void ApplyMove(const Pieces& pieces, BoardStates& board_states) {
		ClearBoardStates(board_states);
		SetPiece(pieces);
		UpdateBoardState(board_states);
	}

	void UpdateBoardState(BoardStates& board_states) {		
		legal_moves_.clear();
		for (auto pieces : pieces_) {
			board_states.insert(std::make_pair(pieces.pos, pieces));
		}
		CalcLegalMoves(board_states);
	}

	bool IsExistJiang() const {
		return std::find_if(pieces_.cbegin(), pieces_.cend(), [](const Pieces &pieces) {
			return pieces.type == PIECE_JIANG;
			}) == pieces_.cend();
	}

	void CalcLegalMoves(BoardStates& board_states) {
		for (auto pieces : pieces_) {
			auto moves = Rules::GetPossibleMoves(pieces, board_states);
#ifdef _DEBUG
			/*
			if (!moves.empty()) {
				std::cout << pieces.color << " " << pieces.type << " max move: " << moves.size() << "\n";
				DebugMoves(moves, pieces.type, board_states);
			}
			*/
#endif
			for (auto move : moves) {
				assert(move.column <= MAX_COL);
				assert(move.row <= MAX_ROW);
				assert(pieces.color == GetColor());
				legal_moves_.emplace(pieces.color, pieces.type, move);
			}
		}		
	}

private:
	void DebugMoves(const std::vector<XiangQiGameMove>& moves, PiecesType type, const BoardStates& board_states) const {
		//if (type != PIECE_PAO1 && type != PIECE_PAO2) {
		//	return;
		//}
		for (auto row = MIN_ROW; row <= MAX_ROW; ++row) {
			for (auto col = MIN_COL; col <= MAX_COL; ++col) {
				XiangQiGameMove move(row, col);
				if (std::find(moves.begin(), moves.end(), move) != moves.end()) {
					std::cout << std::setw(2) << type << "|";
				}
				else {
					auto itr = board_states.find(move);
					if (itr != board_states.end()) {
						std::cout << std::setw(2) << (*itr).second.type << "|";
					}
					else {
						std::cout << std::setw(2) << " " << "|";
					}
				}
			}
			std::cout << "\n";
		}
		std::cout << "\n----------\n";
	}

	int8_t player_id_;	
	std::vector<Pieces> pieces_;
	HashSet<Pieces> legal_moves_;
	BoardStates board_states_;
};

class XiangQiGameState {
public:
	static const int8_t PLAYER1 = 'B';
	static const int8_t PLAYER2 = 'R';
	static const int8_t EMPTY = ' ';

	XiangQiGameState()
		: winner_exists_(false)
		, is_terminal_(false)
		, player_id_(1)
		, agent_(player_id_)
		, opp_agent_(player_id_ == 1 ? 0 : 1) {
		agent_.Initial(board_states_);
		opp_agent_.Initial(board_states_);
		agent_.CalcLegalMoves(board_states_);
		opp_agent_.CalcLegalMoves(board_states_);
	}

	Colors GetColor() const {
		return player_id_ != 1 ? COLOR_BLACK : COLOR_RED;
	}

	void ApplyMove(const Pieces& pieces) {
		auto is_capture = false;

		auto itr = board_states_.find(pieces.pos);
		if (itr != board_states_.end()) {
			if ((*itr).second.color != GetColor()) {
				is_capture = true;
			}
		}

		if (is_capture) {
			GetOppAgent().RemovePieces(pieces, board_states_);
			GetAgent().ApplyMove(pieces, board_states_);
			std::cout << *this << "\n";			
			winner_exists_ = GetOppAgent().IsExistJiang();
			is_terminal_ = winner_exists_;
		} else {
			GetAgent().ApplyMove(pieces, board_states_);
			std::cout << pieces << "\n" << *this << "\n";
		}		
		player_id_ = ((player_id_ == 1) ? 2 : 1);
	}

	double Evaluate() const noexcept {
		if ((is_terminal_ == true) && (winner_exists_ == false)) {
			return 0.0;
		}
		return (player_id_ == 1) ? -1 : 1;
	}	

	int8_t GetPlayerID() const noexcept {
		return player_id_;
	}

	Pieces GetRandomMove() const {
		return GetAgent().GetRandomMove();
	}

	bool IsLegalMove(const Pieces& pieces) const {
		return GetAgent().IsLegalMove(pieces);
	}

	const HashSet<Pieces> & GetLegalMoves() const {
		return GetAgent().GetLegalMoves();
	}

	std::string ToString() const {
		return "";
	}

	int8_t GetWinner() const {
		return player_id_;
	}

	bool IsWinnerExsts() const {
		return winner_exists_;
	}

	bool IsTerminal() const {
		return is_terminal_;
	}

private:
	XiangQiGameAgent& GetOppAgent() {
		if (player_id_ == 1) {
			return opp_agent_;			
		}
		else {
			return agent_;
		}
	}

	XiangQiGameAgent& GetAgent() {
		if (player_id_ == 1) {
			return agent_;
		}
		else {
			return opp_agent_;
		}
	}

	const XiangQiGameAgent& GetAgent() const {
		if (player_id_ == 1) {
			return agent_;
		}
		else {
			return opp_agent_;
		}
	}

	friend std::ostream& operator<<(std::ostream& ostr, const XiangQiGameState& game_state) {
		ostr << game_state.board_states_;
		return ostr;
	}

	bool winner_exists_;
	bool is_terminal_;
	int8_t player_id_;
	BoardStates board_states_;
	XiangQiGameAgent agent_;
	XiangQiGameAgent opp_agent_;
};

}
