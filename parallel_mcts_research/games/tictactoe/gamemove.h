// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <sstream>

namespace tictactoe {

class TicTacToeGameMove {
public:
	TicTacToeGameMove(size_t index = 0) noexcept
		: index(index) {
	}

	~TicTacToeGameMove() noexcept {
	}

	size_t index;

	std::string ToString() const {
		std::ostringstream ostr;
		ostr << index;
		return ostr.str();
	}

private:
	friend bool operator==(const TicTacToeGameMove& lhs, const TicTacToeGameMove& rhs) noexcept;
};

inline bool operator==(const TicTacToeGameMove& lhs, const TicTacToeGameMove& rhs) noexcept {
	return lhs.index == rhs.index;
}

}

namespace std {
	template <>
	class hash<tictactoe::TicTacToeGameMove> {
	public:
		size_t operator()(const tictactoe::TicTacToeGameMove& move) const {
			return move.index;
		}
	};
}
