// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <cassert>
#include <vector>
#include <sstream>

#include "../mcts.h"
#include "pieces.h"
#include "gamemove.h"

namespace xiangqi {

using namespace mcts;

using BoardStates = phmap::flat_hash_map<XiangQiGameMove, Pieces>;

class XiangQiGameState {
public:
	static const int32_t MIN_ROW = 1;
	static const int32_t MIN_COL = 1;

	static const int32_t MAX_ROW = 10;
	static const int32_t MAX_COL = 9;

	static const int8_t PLAYER1 = 'B';
	static const int8_t PLAYER2 = 'R';
	static const int8_t EMPTY = ' ';

	XiangQiGameState()
		: winner_exists_(false)
		, is_terminal_(false)
		, player_id_(1) {
		pieces_ = GetRedPieces();
		opp_pieces_ = GetBlackPieces();
		for (auto pieces : pieces_) {
			state_.insert(std::make_pair(pieces.pos, pieces));
		}
		for (auto pieces : opp_pieces_) {
			state_.insert(std::make_pair(pieces.pos, pieces));
		}
	}

	void ApplyMove(const XiangQiGameMove& move) {
	}

	double Evaluate() const noexcept {
		return 0.0;
	}

	XiangQiGameMove GetRandomMove() const {
		return XiangQiGameMove();
	}

	int8_t GetPlayerID() const noexcept {
		return player_id_;
	}

	bool IsLegalMove(const XiangQiGameMove& move) const {
		return false;
	}

	HashSet<XiangQiGameMove> GetLegalMoves() const noexcept {
		HashSet<XiangQiGameMove> legal_moves;
		return legal_moves;
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
	static const HashSet<Pieces>& GetRedPieces() {
		static const HashSet<Pieces> pieces {
			{ PiecesType::PIECE_CHE,   {1, 1} },
			{ PiecesType::PIECE_CHE,   {1, 9} },
			{ PiecesType::PIECE_PAO,   {3, 2} },
			{ PiecesType::PIECE_PAO,   {3, 8} },
			{ PiecesType::PIECE_MA,    {1, 2} },
			{ PiecesType::PIECE_MA,    {1, 8} },
			{ PiecesType::PIECE_XIANG, {1, 3} },
			{ PiecesType::PIECE_XIANG, {1, 7} },
			{ PiecesType::PIECE_SHI,   {1, 4} },
			{ PiecesType::PIECE_SHI,   {1, 6} },
			{ PiecesType::PIECE_BING,  {4, 1} },
			{ PiecesType::PIECE_BING,  {4, 3} },
			{ PiecesType::PIECE_BING,  {4, 5} },
			{ PiecesType::PIECE_BING,  {4, 7} },
			{ PiecesType::PIECE_BING,  {4, 9} },
			{ PiecesType::PIECE_JIANG, {1, 5} },
		};
		return pieces;
	}

	static const HashSet<Pieces>& GetBlackPieces() {
		static const HashSet<Pieces> pieces{
			{ PiecesType::PIECE_CHE,   {10, 1} },
			{ PiecesType::PIECE_CHE,   {10, 9} },
			{ PiecesType::PIECE_PAO,   {8,  2} },
			{ PiecesType::PIECE_PAO,   {8,  8} },
			{ PiecesType::PIECE_MA,    {10, 2} },
			{ PiecesType::PIECE_MA,    {10, 8} },
			{ PiecesType::PIECE_XIANG, {10, 3} },
			{ PiecesType::PIECE_XIANG, {10, 7} },
			{ PiecesType::PIECE_SHI,   {10, 4} },
			{ PiecesType::PIECE_SHI,   {7,  6} },
			{ PiecesType::PIECE_BING,  {7,  1} },
			{ PiecesType::PIECE_BING,  {7,  3} },
			{ PiecesType::PIECE_BING,  {7,  5} },
			{ PiecesType::PIECE_BING,  {7,  7} },
			{ PiecesType::PIECE_BING,  {7,  9} },
			{ PiecesType::PIECE_JIANG, {10, 5} },
		};
		return pieces;
	}

	static std::vector<XiangQiGameMove> MovesOnSameLine(int32_t row, int32_t col, const BoardStates &board) {
		std::vector<XiangQiGameMove> moves;
		
		for (auto i = row; i < MAX_ROW; ++i) {
			XiangQiGameMove move(i, col);
			if (board.find(move) != board.end()) {
				moves.push_back(move);
				break;
			}
			moves.push_back(move);
		}
		
		for (auto i = row; i >= MIN_ROW; --i) {
			XiangQiGameMove move(i, col);
			if (board.find(move) != board.end()) {
				moves.push_back(move);
				break;
			}
			moves.push_back(move);
		}
		
		for (auto i = col; i < MAX_COL; ++i) {
			XiangQiGameMove move(row, i);
			if (board.find(move) != board.end()) {
				moves.push_back(move);
				break;
			}
			moves.push_back(move);
		}

		for (auto i = col; i >= MIN_COL; --i) {
			XiangQiGameMove move(row, i);
			if (board.find(move) != board.end()) {
				moves.push_back(move);
				break;
			}				
			moves.push_back(move);
		}
		
		return moves;
	}

	std::vector<XiangQiGameMove> GetCheLegalMove(int32_t row, int32_t col, const BoardStates& board) const {
		return MovesOnSameLine(row, col, board);
	}

	std::vector<XiangQiGameMove> GetMaLegalMove(int32_t row, int32_t col, const BoardStates& board) const {
		std::vector<XiangQiGameMove> moves;
		if (board.find(XiangQiGameMove(row + 1, col)) == board.end()) {
			moves.emplace_back(row + 2, col + 1);
			moves.emplace_back(row + 2, col - 1);
		}
		if (board.find(XiangQiGameMove(row - 1, col)) == board.end()) {
			moves.emplace_back(row - 2, col + 1);
			moves.emplace_back(row - 2, col - 1);
		}
		if (board.find(XiangQiGameMove(row, col + 1)) == board.end()) {
			moves.emplace_back(row + 1, col + 2);
			moves.emplace_back(row - 1, col + 2);
		}
		if (board.find(XiangQiGameMove(row, col - 1)) == board.end()) {
			moves.emplace_back(row + 1, col - 2);
			moves.emplace_back(row + 1, col - 2);
		}
		return moves;
	}

	std::vector<XiangQiGameMove> GetPaoLegalMove(int32_t row, int32_t col, const BoardStates& board) const {
		std::vector<XiangQiGameMove> moves;
		return moves;
	}

	template <typename Function>
	void FindFirstOpponentOnRow(int32_t row, int32_t start_col, const XiangQiGameState& state, Function &&f) {
		while (start_col >= 0; start_col < MAX_COL) {
			XiangQiGameMove move(row, start_col);
			if (state.find(move) == state.end()) {

			}
		}
	}

	friend std::ostream& operator<<(std::ostream& ostr, const XiangQiGameState& state) {
		for (auto row = MIN_ROW; row <= MAX_ROW; ++row) {
			for (auto col = MIN_COL; col <= MAX_COL; ++col) {
				XiangQiGameMove move(col, row);
				auto itr = state.state_.find(move);
				assert(itr != state.state_.end());
				ostr << (*itr).second.type << "|";
			}
			ostr << "\n";
		}		
		return ostr;
	}

	bool winner_exists_;
	bool is_terminal_;
	int8_t player_id_;	
	BoardStates state_;
	HashSet<Pieces> pieces_;
	HashSet<Pieces> opp_pieces_;
};

}
