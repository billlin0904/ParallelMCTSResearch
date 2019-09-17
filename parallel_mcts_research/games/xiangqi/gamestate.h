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

#define DEBUG_LEGAL_MOVES 0

static const int32_t MIN_ROW = 1;
static const int32_t MIN_COL = 1;

static const int32_t MAX_ROW = 10;
static const int32_t MAX_COL = 9;

using BoardStates = HashMap<XiangQiGameMove, Pieces>;

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
	ostr << "\n";
	return ostr;
}

struct Rules {
	static const std::vector<Pieces>& GetRedPieces();

	static const std::vector<Pieces>& GetBlackPieces();

	static bool IsCaptureOppJiang(const Pieces& pieces, const BoardStates& board);

	static LegalMoves GetPossibleMoves(const Pieces& pieces, const BoardStates& board);
};

class XiangQiGameAgent {
public:
	explicit XiangQiGameAgent(int8_t player_id, BoardStates& board_states)
		: player_id_(player_id) {
		if (player_id_ == 1) {
			pieces_ = Rules::GetRedPieces();
		}
		else {
			pieces_ = Rules::GetBlackPieces();
		}

		for (auto pieces : pieces_) {
			board_states.insert(std::make_pair(pieces.pos, pieces));
		}
		ProcessLegalMoves(board_states);
	}

	Colors GetColor() const noexcept {
		return player_id_ != 1 ? COLOR_BLACK : COLOR_RED;
	}

	bool IsLegalMoveEmpty() const {
		return legal_moves_.empty();
	}

	const HashSet<Pieces>& GetLegalMoves() const {
		return legal_moves_;
	}

	bool IsLegalMove(const Pieces& pieces) const {
		return legal_moves_.find(pieces) != legal_moves_.end();
	}

	Pieces GetRandomMove() const {
		auto itr = std::next(std::begin(legal_moves_),
			RNG::Get()(0, int32_t(legal_moves_.size() - 1)));
		return *itr;
	}

	void RemovePieces(const Pieces& pieces, BoardStates& board_states) {
		ClearBoardStates(board_states);
		auto itr = std::find_if(pieces_.begin(), pieces_.end(), [&pieces](const Pieces& other) {
			return pieces.pos == other.pos;
			});
		assert(itr != pieces_.end());
		pieces_.erase(itr);
		UpdateBoardState(board_states);
	}

	void SetPiece(const Pieces& pieces) {
		auto itr = std::find_if(pieces_.begin(), pieces_.end(),
			[pieces](const Pieces& other) {
				return pieces.type == other.type;
			});
		assert(itr != pieces_.end());
		(*itr) = pieces;
	}

	void ApplyMove(const Pieces& pieces, BoardStates& board_states) {
		ClearBoardStates(board_states);
		SetPiece(pieces);
		UpdateBoardState(board_states);
		assert(!pieces_.empty());
	}	

	bool IsExistJiang() const {
		return std::find_if(pieces_.cbegin(), pieces_.cend(), [](const Pieces& pieces) {
			return pieces.type == PIECES_JIANG;
			}) != pieces_.cend();
	}

	bool IsCaptureOppJiang(const BoardStates& board_states) const {
		auto itr = std::find_if(pieces_.cbegin(), pieces_.cend(), [](const Pieces& pieces) {
			return pieces.type == PIECES_JIANG;
			});
		assert(itr != pieces_.end());
		return Rules::IsCaptureOppJiang((*itr), board_states);
	}

private:
	void ProcessLegalMoves(BoardStates& board_states) {		
		for (auto pieces : pieces_) {
			const auto moves = Rules::GetPossibleMoves(pieces, board_states);
#if DEBUG_LEGAL_MOVES
			if (!moves.empty()) {
				std::cout << pieces.color << pieces.type << " max move: " << moves.size() << "\n";
				DebugMoves(moves, pieces.color, pieces.type, board_states);
			}
#endif
			for (auto move : moves) {
				Pieces temp = pieces;
				temp.pos = move;
				legal_moves_.insert(temp);
			}
		}
	}

	void CalcLegalMoves(BoardStates& board_states) {
		legal_moves_.clear();
		ProcessLegalMoves(board_states);
	}

	void UpdateBoardState(BoardStates& board_states) {
		for (auto pieces : pieces_) {
			board_states.insert(std::make_pair(pieces.pos, pieces));
		}
		CalcLegalMoves(board_states);
	}

	void ClearBoardStates(BoardStates & board_states) {
		for (auto pieces : pieces_) {
			auto itr = board_states.find(pieces.pos);
			if (itr == board_states.end()) {
				continue;
			}
			assert(pieces.color == GetColor());
			board_states.erase(pieces.pos);
		}
	}

	void DebugMoves(const LegalMoves& moves, Colors color, PiecesType type, const BoardStates& board_states) const {
		if (type != PIECES_JIANG && type != PIECES_CHE1) {
			return;
		}
		for (auto row = MIN_ROW; row <= MAX_ROW; ++row) {
			for (auto col = MIN_COL; col <= MAX_COL; ++col) {
				XiangQiGameMove move(row, col);
				if (std::find(moves.begin(), moves.end(), move) != moves.end()) {
					std::cout << color << type << "|";
				}
				else {
					auto itr = board_states.find(move);
					if (itr != board_states.end()) {
						std::cout << (*itr).second.color << (*itr).second.type << "|";
					}
					else {
						std::cout << std::setw(3) << "   " << "|";
					}
				}
			}
			std::cout << "\n";
		}
		std::cout << "\n";
	}

	int8_t player_id_;	
	std::vector<Pieces> pieces_;
	HashSet<Pieces> legal_moves_;
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
		, agent_(player_id_, board_states_)
		, opp_agent_(player_id_ == 1 ? 2 : 1, board_states_) {		
	}

	Colors GetColor() const noexcept {
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
			//std::cout << *this << "\n";
			winner_exists_ = !GetOppAgent().IsExistJiang();
			is_terminal_ = winner_exists_;
			
		} else {
			GetAgent().ApplyMove(pieces, board_states_);
			//std::cout << pieces << "\n" << *this << "\n";
			winner_exists_ = GetAgent().IsCaptureOppJiang(board_states_);
			is_terminal_ = winner_exists_;			
		}

#ifdef _DEBUG
		if (GetAgent().IsLegalMoveEmpty() || GetOppAgent().IsLegalMoveEmpty()) {
			//std::cout << pieces << "\n" << *this << "\n";
			assert(is_terminal_);
		}

		if (!winner_exists_ || !is_terminal_) {
			assert(GetAgent().IsExistJiang());
			assert(GetOppAgent().IsExistJiang());
		}
#endif

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

	const HashSet<Pieces>& GetLegalMoves() const {
		return GetAgent().GetLegalMoves();
	}

	std::string ToString() const {
		std::ostringstream ostr;
		ostr << *this;
		return ostr.str();
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
