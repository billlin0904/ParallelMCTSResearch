// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <sstream>

namespace tictactoe {

struct TicTacToeGameMove {
	explicit TicTacToeGameMove(size_t index = 0) noexcept
		: index(index) {
	}

	size_t index;

private:
	friend std::ostream& operator<<(std::ostream& ostr, const TicTacToeGameMove& move);
	friend bool operator==(const TicTacToeGameMove& lhs, const TicTacToeGameMove& rhs) noexcept;
};

inline bool operator==(const TicTacToeGameMove& lhs, const TicTacToeGameMove& rhs) noexcept {
	return lhs.index == rhs.index;
}

inline std::ostream& operator<<(std::ostream& ostr, const TicTacToeGameMove& move) {
	ostr << move.index;
	return ostr;
}

}

namespace std {
	template <>
	struct hash<tictactoe::TicTacToeGameMove> {
		size_t operator()(const tictactoe::TicTacToeGameMove& move) const noexcept {
			return move.index;
		}
	};
}
