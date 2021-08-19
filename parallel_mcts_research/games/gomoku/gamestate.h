// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <cassert>
#include <cstdint>
#include <sstream>
#include <iomanip>

#include "../../mcts.h"
#include "../../rng.h"

#include "gamemove.h"

namespace gomoku {

using namespace mcts;

inline constexpr int32_t kMaxWidth = 10;
inline constexpr int32_t kMaxHeight = 10;

class GomokuGameState {
public:
    static constexpr int8_t kPlayer1 = 'O';
    static constexpr int8_t kPlayer2 = '@';
    static constexpr int8_t kEmpty = ' ';

	GomokuGameState() noexcept
		: winner_exists_(false)
		, is_terminal_(false)
		, player_id_(1)
        , remain_move_(kMaxWidth * kMaxHeight)
        , board_(kMaxHeight) {
        legal_moves_.reserve(kMaxWidth * kMaxHeight);
        for (auto row = 0; row < kMaxWidth; ++row) {
            for (auto col = 0; col < kMaxHeight; ++col) {
                board_[row].push_back(kEmpty);
				legal_moves_.emplace(row, col);
			}
		}
	}

	bool IsEmptyMove() const noexcept {
        return remain_move_ == kMaxWidth * kMaxHeight;
	}

	bool IsWinnerExsts() const noexcept {
		return winner_exists_;
	}

	void ApplyMove(const GomokuGameMove& move) {
		if (player_id_ == 1) {
            board_[move.row][move.column] = kPlayer1;
		}
		else {
            board_[move.row][move.column] = kPlayer2;
		}

		legal_moves_.erase(move);

		--remain_move_;
        if (CheckWinner(move) != kEmpty) {
			winner_exists_ = true;
			is_terminal_ = true;
		}
		else {
			CheckTerminal();
		}
		player_id_ = ((player_id_ == 1) ? 2 : 1);
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
        assert(move.row < kMaxWidth && move.column < kMaxHeight);
		assert(!is_terminal_ && !winner_exists_);
        return board_[move.row][move.column] == kEmpty;
	}

	const HashSet<GomokuGameMove>& GetLegalMoves() const noexcept {
		return legal_moves_;
	}

	std::string ToString() const {
		std::ostringstream ostr;
        for (auto row = 0; row < kMaxWidth; ++row) {
            for (auto col = 0; col < kMaxHeight; ++col) {
				ostr << board_[row][col];
			}
		}
		return ostr.str();
	}

	int8_t GetWinner() const {
        for (auto row = 0; row < kMaxWidth; ++row) {
            for (auto col = 0; col < kMaxHeight; ++col) {
                if (board_[row][col] != kEmpty) {
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
        return kEmpty;
	}

private:
	int8_t CheckWinner(const GomokuGameMove& move) const noexcept {
        if (board_[move.row][move.column] != kEmpty) {
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
        return kEmpty;
	}

	int8_t Count(int8_t player, int32_t row, int32_t col, int32_t dirX, int32_t dirY) const noexcept {
		auto ct = 1;

		auto r = row + dirX;
		auto c = col + dirY;

        while (r >= 0 && r < kMaxWidth && c >= 0 && c < kMaxHeight && board_[r][c] == player) {
			ct++;
			r += dirX;
			c += dirY;
		}

		r = row - dirX;
		c = col - dirY;

        while (r >= 0 && r < kMaxWidth && c >= 0 && c < kMaxHeight && board_[r][c] == player) {
			ct++;
			r -= dirX;
			c -= dirY;
		}
		return ct;
	}

	friend std::ostream& operator<<(std::ostream& ostr, const GomokuGameState& state) {
        for (auto row = 0; row < kMaxWidth; ++row) {
            for (auto col = 0; col < kMaxHeight; ++col) {
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
	HashSet<GomokuGameMove> legal_moves_;
	std::vector<std::vector<int8_t>> board_;
};

}
