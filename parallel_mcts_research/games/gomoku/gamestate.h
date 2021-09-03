// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <cassert>
#include <cstdint>
#include <bitset>
#include <iomanip>
#include <vector>
#include <optional>

#include "../../mcts.h"
#include "../../rng.h"

#include "gamemove.h"

namespace gomoku {

using namespace mcts;
inline constexpr int32_t kBoardSize = 9;
inline constexpr int32_t kMaxWidth = kBoardSize;
inline constexpr int32_t kMaxHeight = kBoardSize;

enum Dir {
	kStyle3,
	kStyle2,
	kStyle4,
	kStyle1,
};

class GomokuGameState {
public:
    static constexpr int8_t kPlayer1 = 'O';
    static constexpr int8_t kPlayer2 = 'X';
    static constexpr int8_t kEmpty = '.';

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

		player_moves_[GetCurrentPlayer()].push_back(move);
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
		//std::cout << *this << std::endl;
		if (const auto best_move = GetBestMove()) {
			return best_move.value();
		}
		auto const& legal_moves = GetLegalMoves();
		assert(!legal_moves.empty());
		auto itr = std::next(std::begin(legal_moves), RNG::Get()(0, static_cast<int32_t>(legal_moves.size() - 1)));
		return *itr;
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

    [[nodiscard]] int8_t GetCurrentPlayer() const noexcept {
		return player_id_ == kPlayerID ? kPlayer1 : kPlayer2;
	}

    [[nodiscard]] std::optional<GomokuGameMove> GetBestMove() const {
	    const auto start = GetCurrentPlayer();
		std::vector<std::tuple<int32_t, GomokuGameMove, Dir>> best_moves;
		best_moves.reserve(kMaxWidth * kMaxHeight);

		for (const auto move : player_moves_.find(start)->second) {
			auto count = AnyCount(start, move.row, move.column, 1, 0);
			if (count >= 4) {
				best_moves.emplace_back(count, move, kStyle1);
			}
			count = AnyCount(start, move.row, move.column, 0, 1);
			if (count >= 4) {
				best_moves.emplace_back(count, move, kStyle2);
			}
			count = AnyCount(start, move.row, move.column, 1, -1);
			if (count >= 4) {
				best_moves.emplace_back(count, move, kStyle3);
			}
			count = AnyCount(start, move.row, move.column, 1, 1);
			if (count >= 4) {
				best_moves.emplace_back(count, move, kStyle4);
			}
		}
		auto itr = std::max_element(best_moves.begin(), 
			best_moves.end(), [](const auto& first,
			const auto& last) noexcept {
				return std::get<0>(first) > std::get<0>(last);
			});
		if (itr == best_moves.end()) {
			return std::nullopt;
		}
		auto best_move = std::get<1>((*itr));
		switch (std::get<2>((*itr))) {
		case kStyle1:
			return FindEmpty(best_move.row, best_move.column, 1, 0);
			break;
		case kStyle2:
			return FindEmpty(best_move.row, best_move.column, 0, 1);
			break;
		case kStyle3:
			return FindEmpty(best_move.row, best_move.column, 1, -1);
			break;
		case kStyle4:
			return FindEmpty(best_move.row, best_move.column, 1, 1);
			break;
		}
		throw std::logic_error("Not found move");
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

	[[nodiscard]] GomokuGameMove FindEmpty(int32_t row, int32_t col, int32_t dir_x, int32_t dir_y) const {
		auto r = row + dir_x;
		auto c = col + dir_y;

		auto i = 0;

		while (r >= 0 && r < kMaxWidth && c >= 0 && c < kMaxHeight && i < 5) {
			if (board_[r][c] == kEmpty) {
				return GomokuGameMove(r, c);
			}
			r += dir_x;
			c += dir_y;
			i++;
		}

		r = row - dir_x;
		c = col - dir_y;

		i = 0;

		while (r >= 0 && r < kMaxWidth && c >= 0 && c < kMaxHeight && i < 5) {
			if (board_[r][c] == kEmpty) {
				return GomokuGameMove(r, c);
			}
			r -= dir_x;
			c -= dir_y;
			i++;
		}

		throw std::logic_error("Not found move");
	}

	[[nodiscard]] int32_t AnyCount(int8_t player, int32_t row, int32_t col, int32_t dir_x, int32_t dir_y) const noexcept {
		auto ct = 1;

		auto r = row + dir_x;
		auto c = col + dir_y;

		auto i = 0;
		auto found_empty = false;

		while (r >= 0 && r < kMaxWidth && c >= 0 && c < kMaxHeight && i < 5) {
			//std::cout << r << "," << c << std::endl;
			if (board_[r][c] == player) {
				ct++;
			} else if (board_[r][c] == kEmpty) {
				found_empty = true;
			} else {
				ct--;
			}
			r += dir_x;
			c += dir_y;
			i++;			
		}

		r = row - dir_x;
		c = col - dir_y;

		i = 0;

		while (r >= 0 && r < kMaxWidth && c >= 0 && c < kMaxHeight && i < 5) {
			//std::cout << r << "," << c << std::endl;
			if (board_[r][c] == player) {
				ct++;
			}
			else if (board_[r][c] == kEmpty) {
				found_empty = true;
			} else {
				ct--;
			}
			r -= dir_x;
			c -= dir_y;
			i++;
		}

		if (!found_empty) {
			return 0;
		}
		return ct;		
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
		ostr << "  ";
		for (auto row = 0; row < kMaxWidth; ++row) {
			ostr << std::right << std::setfill(' ') << std::setw(2) << static_cast<char>('A' + row);
		}
		ostr << std::endl;
        for (auto row = 0; row < kMaxWidth; ++row) {
			ostr << std::right << std::setfill(' ') << std::setw(2) << row;
            for (auto col = 0; col < kMaxHeight; ++col) {
				ostr << std::right << std::setfill(' ') << std::setw(2) << state.board_[col][row];
			}
			ostr << std::endl;
		}
		return ostr;
	}

	bool winner_exists_;
	bool is_terminal_;
	int8_t player_id_;
	int32_t remain_move_;
	HashSet<GomokuGameMove> legal_moves_;
	std::vector<std::vector<int8_t>> board_;
	std::map<int8_t, std::vector<GomokuGameMove>> player_moves_;
	
};

}
