// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <cstdint>
#include <array>
#include <iostream>

#include "../../mcts.h"
#include "../../rng.h"
#include "../../tweakme.h"
#include "gamemove.h"

namespace tictactoe {

using namespace mcts;

class TicTacToeGameState {
public:
    static constexpr int8_t kPlayer1 = 'O';
    static constexpr int8_t kPlayer2 = 'X';
    static constexpr int8_t kEmpty = ' ';

	TicTacToeGameState()
		: winner_exists_(false)
		, is_terminal_(false)
		, player_id_(kPlayerID)
		, board_() {
        board_.fill(kEmpty);
	}

	[[nodiscard]] bool IsTerminal() const noexcept {
		return is_terminal_;
	}

	[[nodiscard]] bool IsWinnerExist() const noexcept {
		return winner_exists_;
	}

	void ApplyMove(const TicTacToeGameMove& move) {
		if (player_id_ == kPlayerID) {
            board_.at(move.index) = kPlayer1;
		}
		else {
            board_.at(move.index) = kPlayer2;
		}

        if (CheckWinner() != kEmpty) {
			winner_exists_ = true;
			is_terminal_ = true;
		} else {
            auto itr = std::find(board_.cbegin(), board_.cend(), kEmpty);
			if (itr == board_.end()) {
				is_terminal_ = true;
			}
		}
		player_id_ = ((player_id_ == kPlayerID) ? kOpponentID : kPlayerID);
		last_move_ = move;
	}

	[[nodiscard]] double Evaluate() const noexcept {
		if ((is_terminal_ == true) && (winner_exists_ == false)) {
			return 0;
		}
		return (player_id_ == kPlayerID) ? -1 : 1;
	}

    [[nodiscard]] TicTacToeGameMove GetRandomMove() const {
		//auto legal_moves = GetLegalMoves();
		//auto itr = std::next(std::begin(legal_moves), RNG::Get()(0, static_cast<int32_t>(legal_moves.size() - 1)));
		//return *itr;
		auto legal_moves = GetLegalMoves();
		std::vector<TicTacToeGameMove> temp(legal_moves.begin(), legal_moves.end());
		RNG::Get().Shuffle(temp.begin(), temp.end());
		return temp.front();
	}

	[[nodiscard]] int8_t GetPlayerID() const noexcept {
		return player_id_;
	}

	[[nodiscard]] bool IsLegalMove(const TicTacToeGameMove& move) const {
		if (board_.size() < move.index) {
			return false;
		}
        return board_.at(move.index) == kEmpty;
	}

	[[nodiscard]] HashSet<TicTacToeGameMove> GetLegalMoves() const noexcept {
		HashSet<TicTacToeGameMove> legal_moves;
		int32_t i = 0;
		for (auto c : board_) {
            if (c == kEmpty) {
				legal_moves.emplace(i);
			}
			i++;
		}
		return legal_moves;
	}

	[[nodiscard]] int8_t CheckWinner() const noexcept {
		for (auto i = 0; i < 9; i += 3) {
			if (board_[i] == board_[i + 1] && board_[i + 1] == board_[i + 2] && board_[i]) {
				return board_[i];
			}
		}

		for (auto i = 0; i < 3; i++) {
			if (board_[i] == board_[i + 3] && board_[i + 3] == board_[i + 6] && board_[i]) {
				return board_[i];
			}
		}

		if (board_[0] == board_[4] && board_[4] == board_[8] && board_[0]) {
			return board_[0];
		}

		if (board_[2] == board_[4] && board_[4] == board_[6] && board_[2]) {
			return board_[2];
		}

        return kEmpty;
	}

	[[nodiscard]] int8_t GetWinner() const {
		return player_id_;
	}

private:
	friend std::ostream& operator<<(std::ostream& ostr, const TicTacToeGameState& state) {
		for (auto i = 0; i < 3; ++i) {
			auto idx = 3 * i;
			ostr << state.board_[idx] << " | " << state.board_[idx + 1] << " | " << state.board_[idx + 2] << '\n';
			std::cout << "---------" << '\n';
		}
		return ostr;
	}

	bool winner_exists_;
	bool is_terminal_;
	int8_t player_id_;
	TicTacToeGameMove last_move_;
	std::array<int8_t, 9> board_;
};

}
