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
using PiecesLegalMoves = HashMap<PiecesType, LegalMoves>;

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

struct Rules {
	static const std::vector<Pieces>& GetRedPieces();

	static const std::vector<Pieces>& GetBlackPieces();

	static bool IsCaptureOppJiang(const Pieces& pieces, const BoardStates& board);

	static LegalMoves GetPossibleMoves(const Pieces& pieces, const BoardStates& board);
};

class XiangQiGameAgent {
public:
	explicit XiangQiGameAgent(int8_t player_id)
		: player_id_(player_id) {
		if (player_id_ == 1) {
			pieces_ = Rules::GetRedPieces();
		}
		else {
			pieces_ = Rules::GetBlackPieces();
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

	void ClearBoardStates(BoardStates& board_states) {
		for (auto pieces : pieces_) {
			auto itr = board_states.find(pieces.pos);
			if (itr == board_states.end()) {
				continue;
			}
			assert(pieces.color == GetColor());
			board_states.erase(pieces.pos);
		}
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

	bool IsLegalMoveEmpty() const {
		return pieces_legal_moves_.empty();
	}

	const HashSet<Pieces>& GetLegalMoves() const {
		return legal_moves_;
	}

	bool IsLegalMove(const Pieces& pieces) const {
		auto itr = pieces_legal_moves_.find(pieces.type);
		if (itr == pieces_legal_moves_.end()) {
			return false;
		}
		return std::find_if((*itr).second.cbegin(), (*itr).second.cend(), [pieces](auto pos) {
			return pieces.pos == pos;
			}) != (*itr).second.cend();
	}

	// 選擇其中棋子開始模擬
	Pieces GetRandomMove() const {
		auto itr = std::next(std::begin(legal_pieces_),
			RNG::Get()(0, int32_t(legal_pieces_.size() - 1)));
		return *itr;
	}

	XiangQiGameMove GetRandomMove(const Pieces& pieces) const {
		auto itr = pieces_legal_moves_.find(pieces.type);
		assert(itr != pieces_legal_moves_.end());
		const auto moves = (*itr).second;
		assert(!moves.empty());
		auto idx = RNG::Get()(0, int32_t(moves.size() - 1));
		return moves[idx];
	}

	void SetPiece(const Pieces& pieces, const BoardStates& board_states) {
		auto itr = std::find_if(pieces_.begin(), pieces_.end(),
			[pieces](const Pieces& other) {
				return pieces.type == other.type;
			});
		assert(itr != pieces_.end());
		(*itr).pos = GetRandomMove(pieces);
		(*itr).type = pieces.type;
		(*itr).color = pieces.color;
	}

	void ApplyMove(const Pieces& pieces, BoardStates& board_states) {
		ClearBoardStates(board_states);
		SetPiece(pieces, board_states);
		UpdateBoardState(board_states);
		assert(!pieces_.empty());
	}

	void UpdateBoardState(BoardStates& board_states) {		
		for (auto pieces : pieces_) {
			board_states.insert(std::make_pair(pieces.pos, pieces));
		}
		CalcLegalMoves(board_states);
	}

	bool IsExistJiang() const {
		return std::find_if(pieces_.cbegin(), pieces_.cend(), [](const Pieces &pieces) {
			return pieces.type == PIECES_JIANG;
			}) != pieces_.cend();
	}

	bool IsCaptureOppJiang(const BoardStates& board_states) const {
		auto itr = std::find_if(pieces_.cbegin(), pieces_.cend(), [](const Pieces& pieces) {
			return pieces.type == PIECES_JIANG;
			});
		return Rules::IsCaptureOppJiang((*itr), board_states);
	}

	void CalcLegalMoves(BoardStates& board_states) {
		legal_moves_.clear();		
		legal_pieces_.clear();
		pieces_legal_moves_.clear();
		for (auto& pieces : pieces_) {
			const auto moves = Rules::GetPossibleMoves(pieces, board_states);
#if DEBUG_LEGAL_MOVES
			if (!moves.empty()) {
				std::cout << pieces.color << pieces.type << " max move: " << moves.size() << "\n";
				DebugMoves(moves, pieces.color, pieces.type, board_states);
			}
#endif
			if (!moves.empty()) {
				legal_pieces_.push_back(pieces);
				pieces_legal_moves_[pieces.type] = moves;
				legal_moves_.insert(pieces);
			}			
		}
	}

private:
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
		std::cout << "\n----------\n";
	}

	int8_t player_id_;	
	std::vector<Pieces> pieces_;
	std::vector<Pieces> legal_pieces_;
	PiecesLegalMoves pieces_legal_moves_;
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
		, agent_(player_id_)
		, opp_agent_(player_id_ == 1 ? 2 : 1) {
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
			//std::cout << *this << "\n";
			winner_exists_ = !GetOppAgent().IsExistJiang();
			is_terminal_ = winner_exists_;
			
		} else {
			GetAgent().ApplyMove(pieces, board_states_);
			winner_exists_ = GetAgent().IsCaptureOppJiang(board_states_);
			is_terminal_ = winner_exists_;
			//std::cout << pieces << "\n" << *this << "\n";
		}

#ifdef _DEBUG
		if (GetAgent().IsLegalMoveEmpty() || GetOppAgent().IsLegalMoveEmpty()) {
			std::cout << pieces << "\n" << *this << "\n";
			assert(is_terminal_);
		}
#endif

		if (!winner_exists_ || !is_terminal_) {
			assert(GetAgent().IsExistJiang());
			assert(GetOppAgent().IsExistJiang());
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

	const HashSet<Pieces>& GetLegalMoves() const {
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
