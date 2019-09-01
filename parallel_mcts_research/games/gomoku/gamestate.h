#pragma once

#include <cstdint>
#include <sstream>

#include "gamemove.h"
#include "../mcts.h"
#include "../rng.h"

namespace gomoku {

static const int32_t MAX_WIDTH = 10;
static const int32_t MAX_HEIGHT = 10;

static const int8_t PLAYER1 = 'O';
static const int8_t PLAYER2 = '@';
static const int8_t EMPTY = ' ';

using namespace mcts;

class GomokuGameState {
public:
	GomokuGameState() noexcept
		: winner_exists_(false)
		, is_terminal_(false)
		, player_id_(1)
		, remain_move_(MAX_WIDTH * MAX_HEIGHT)
		, board_(MAX_HEIGHT) {
		legal_moves_.reserve(MAX_WIDTH * MAX_HEIGHT);
		for (auto row = 0; row < MAX_WIDTH; ++row) {
			for (auto col = 0; col < MAX_HEIGHT; ++col) {
				board_[row].push_back(EMPTY);
				legal_moves_.emplace(row, col);
			}
		}
	}

	bool IsEmptyMove() const noexcept {
		return remain_move_ == MAX_WIDTH * MAX_HEIGHT;
	}

	bool IsWinnerExsts() const noexcept {
		return winner_exists_;
	}

	void ApplyMove(const GomokuGameMove& move) {
		if (player_id_ == 1) {
			board_[move.row][move.column] = PLAYER1;
		}
		else {
			board_[move.row][move.column] = PLAYER2;
		}

		legal_moves_.erase(move);

		--remain_move_;
		if (CheckWinner(move) != EMPTY) {
			winner_exists_ = true;
			is_terminal_ = true;
		}
		else {
			CheckTerminal();
		}
		player_id_ = ((player_id_ == 1) ? 2 : 1);
		last_move_ = move;
	}

	double Evaluate() const noexcept {
		if ((is_terminal_ == true) && (winner_exists_ == false)) {
			return 0.0;
		}
		return (player_id_ == 1) ? -1 : 1;
	}

	bool IsTerminal() const noexcept {
		return is_terminal_;
	}

	GomokuGameMove GetRandomMove() const {
		const auto& legal_moves = GetLegalMoves();
		assert(!legal_moves.empty());
		auto itr = std::next(std::begin(legal_moves), RNG::Get()(0, int32_t(legal_moves.size() - 1)));
		return *itr;
	}

	int8_t GetPlayerID() const noexcept {
		return player_id_;
	}

	void CheckTerminal() noexcept {
		if (winner_exists_) {
			return;
		}
		is_terminal_ = remain_move_ == 0;
	}

	bool IsLegalMove(const GomokuGameMove& move) const {
		assert(move.row < MAX_WIDTH && move.column < MAX_HEIGHT);
		assert(!is_terminal_ && !winner_exists_);
		return board_[move.row][move.column] == EMPTY;
	}

	const mcts::HashSet<GomokuGameMove>& GetLegalMoves() const noexcept {
		return legal_moves_;
	}

	std::string ToString() const {
		std::ostringstream ostr;
		for (auto row = 0; row < MAX_WIDTH; ++row) {
			for (auto col = 0; col < MAX_HEIGHT; ++col) {
				ostr << board_[row][col];
			}
		}
		return ostr.str();
	}

	int8_t GetWinner() const {
		for (auto row = 0; row < MAX_WIDTH; ++row) {
			for (auto col = 0; col < MAX_HEIGHT; ++col) {
				if (board_[row][col] != EMPTY) {
					if (Count(board_[row][col], row, col, 1, 0) >= 5) {
						return board_[row][col];
					}
					if (Count(board_[row][col], row, col, 0, 1) >= 5) {
						return board_[row][col];
					}
					if (Count(board_[row][col], row, col, 1, -1) >= 5) {
						return board_[row][col];
					}
					if (Count(board_[row][col], row, col, 1, 1) >= 5) {
						return board_[row][col];
					}
				}
			}
		}
		return EMPTY;
	}

private:
	int8_t CheckWinner(const GomokuGameMove& move) const noexcept {
		if (board_[move.row][move.column] != EMPTY) {
			if (Count(board_[move.row][move.column], move.row, move.column, 1, 0) >= 5) {
				return board_[move.row][move.column];
			}
			if (Count(board_[move.row][move.column], move.row, move.column, 0, 1) >= 5) {
				return board_[move.row][move.column];
			}
			if (Count(board_[move.row][move.column], move.row, move.column, 1, -1) >= 5) {
				return board_[move.row][move.column];
			}
			if (Count(board_[move.row][move.column], move.row, move.column, 1, 1) >= 5) {
				return board_[move.row][move.column];
			}
		}
		return EMPTY;
	}

	int8_t Count(int8_t player, int32_t row, int32_t col, int32_t dirX, int32_t dirY) const noexcept {
		auto ct = 1;

		auto r = row + dirX;
		auto c = col + dirY;

		while (r >= 0 && r < MAX_WIDTH && c >= 0 && c < MAX_HEIGHT && board_[r][c] == player) {
			ct++;
			r += dirX;
			c += dirY;
		}

		r = row - dirX;
		c = col - dirY;

		while (r >= 0 && r < MAX_WIDTH && c >= 0 && c < MAX_HEIGHT && board_[r][c] == player) {
			ct++;
			r -= dirX;
			c -= dirY;
		}
		return ct;
	}

	friend std::ostream& operator<<(std::ostream& ostr, const GomokuGameState& state) {
		for (auto row = 0; row < MAX_WIDTH; ++row) {
			for (auto col = 0; col < MAX_HEIGHT; ++col) {
				ostr << state.board_[col][row] << "|";
			}
			ostr << "\n";
		}
		return ostr;
	}

	bool winner_exists_;
	bool is_terminal_;
	int8_t player_id_;
	int32_t remain_move_;
	GomokuGameMove last_move_;
	mcts::HashSet<GomokuGameMove> legal_moves_;
	std::vector<std::vector<int8_t>> board_;
};

}