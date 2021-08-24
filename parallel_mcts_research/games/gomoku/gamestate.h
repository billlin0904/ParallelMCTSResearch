// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <cassert>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <vector>

#include "../../mcts.h"
#include "../../rng.h"

#include "gamemove.h"

namespace gomoku {

using namespace mcts;

inline constexpr int32_t kMaxWidth = 9;
inline constexpr int32_t kMaxHeight = 9;

class GomokuGameState {
public:
    static constexpr int8_t kPlayer1 = 'O';
    static constexpr int8_t kPlayer2 = '@';
    static constexpr int8_t kEmpty = ' ';

	GomokuGameState()
		: winner_exists_(false)
		, is_terminal_(false)
		, player_id_(kPlayerID)
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

    [[nodiscard]] bool IsEmptyMove() const noexcept {
        return remain_move_ == kMaxWidth * kMaxHeight;
	}

	[[nodiscard]] bool IsWinnerExist() const noexcept {
		return winner_exists_;
	}

	void ApplyMove(const GomokuGameMove& move) {
		if (player_id_ == kPlayerID) {
            board_[move.row][move.column] = kPlayer1;
		}
		else {
            board_[move.row][move.column] = kPlayer2;
		}

		auto result = legal_moves_.erase(move);
		assert(result != 0);

		--remain_move_;
        if (CheckWinner(move) != kEmpty) {
			winner_exists_ = true;
			is_terminal_ = true;
		} else {
			CheckTerminal();
		}
		player_id_ = ((player_id_ == kPlayerID) ? kOpponentID : kPlayerID);
	}

	[[nodiscard]] double Evaluate() const noexcept {
		if ((is_terminal_ == true) && (winner_exists_ == false)) {
            return 0;
		}
        return (player_id_ == kPlayerID) ? -1 : 1;
	}

	[[nodiscard]] bool IsTerminal() const noexcept {
		return is_terminal_;
	}

	[[nodiscard]] GomokuGameMove GetRandomMove() const {
		auto const &legal_moves = GetLegalMoves();
		assert(!legal_moves.empty());
		std::vector<GomokuGameMove> temp(legal_moves.begin(), legal_moves.end());		
		RNG::Get().Shuffle(temp.begin(), temp.end());
		return temp.front();
	}

	[[nodiscard]] int8_t GetPlayerID() const noexcept {
		return player_id_;
	}

	void CheckTerminal() noexcept {
		if (winner_exists_) {
			return;
		}
		is_terminal_ = remain_move_ == 0;
	}

	[[nodiscard]] bool IsLegalMove(const GomokuGameMove& move) const {
        assert(move.row < kMaxWidth && move.column < kMaxHeight);
		assert(!is_terminal_ && !winner_exists_);
        return board_[move.row][move.column] == kEmpty;
	}

	[[nodiscard]] const HashSet<GomokuGameMove>& GetLegalMoves() const noexcept {
		return legal_moves_;
	}

	[[nodiscard]] int8_t GetWinner() const {
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
	[[nodiscard]] int8_t CheckWinner(const GomokuGameMove& move) const noexcept {
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

	[[nodiscard]] int8_t Count(int8_t player, int32_t row, int32_t col, int32_t dir_x, int32_t dir_y) const noexcept {
		auto ct = 1;

		auto r = row + dir_x;
		auto c = col + dir_y;

        while (r >= 0 && r < kMaxWidth && c >= 0 && c < kMaxHeight && board_[r][c] == player) {
			ct++;
			r += dir_x;
			c += dir_y;
		}

		r = row - dir_x;
		c = col - dir_y;

        while (r >= 0 && r < kMaxWidth && c >= 0 && c < kMaxHeight && board_[r][c] == player) {
			ct++;
			r -= dir_x;
			c -= dir_y;
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
	int8_t player_id_ : 2;
	int32_t remain_move_;
	HashSet<GomokuGameMove> legal_moves_;
	std::vector<std::vector<int8_t>> board_;
};

}
