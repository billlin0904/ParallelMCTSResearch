// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <cstdint>

#include "../mcts.h"
#include "../rng.h"
#include "../tweakme.h"
#include "gamemove.h"

namespace tictactoe {

using namespace mcts;

class TicTacToeGameState {
public:
	static const int8_t PLAYER1 = 'O';
	static const int8_t PLAYER2 = 'X';
	static const int8_t EMPTY = ' ';

	TicTacToeGameState() noexcept
		: winner_exists_(false)
		, is_terminal_(false)
		, player_id_(1) {
		board_.fill(EMPTY);
	}

	bool IsTerminal() const noexcept {
		return is_terminal_;
	}

	bool IsWinnerExsts() const noexcept {
		return winner_exists_;
	}

	void ApplyMove(const TicTacToeGameMove& move) {
		if (player_id_ == 1) {
			board_.at(move.index) = PLAYER1;
		}
		else {
			board_.at(move.index) = PLAYER2;
		}

		if (CheckWinner() != EMPTY) {
			winner_exists_ = true;
			is_terminal_ = true;
		}
		else {
			auto itr = std::find(board_.begin(), board_.end(), EMPTY);
			if (itr == board_.end()) {
				is_terminal_ = true;
			}
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

	TicTacToeGameMove GetRandomMove() const {
		auto legal_moves = GetLegalMoves();
		auto itr = std::next(std::begin(legal_moves), RNG::Get()(0, int32_t(legal_moves.size() - 1)));
		return *itr;
	}

	int8_t GetPlayerID() const noexcept {
		return player_id_;
	}

	bool IsLegalMove(const TicTacToeGameMove& move) const {
		if (board_.size() < move.index) {
			return false;
		}
		return board_.at(move.index) == EMPTY;
	}

	HashSet<TicTacToeGameMove> GetLegalMoves() const noexcept {
		HashSet<TicTacToeGameMove> legal_moves;
		int32_t i = 0;
		for (auto c : board_) {
			if (c == EMPTY) {
				legal_moves.emplace(i);
			}
			i++;
		}
		return legal_moves;
	}

	int8_t CheckWinner() const noexcept {
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

		return EMPTY;
	}

	std::string ToString() const {
		std::ostringstream ostr;
		for (auto i = 0; i < 3; ++i) {
			auto idx = 3 * i;
			ostr << board_[idx] << board_[idx + 1] << board_[idx + 2];
		}
		return ostr.str();
	}

	int8_t GetWinner() const {
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
