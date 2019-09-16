// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <cstdint>
#include <utility>
#include <cstdlib>
#include <string>

namespace gomoku {

class GomokuGameMove {
public:
	GomokuGameMove(int8_t row = 0, int8_t column = 0) noexcept
		: row(row)
		, column(column) {
	}

	~GomokuGameMove() noexcept {
	}

	int8_t row;
	int8_t column;

	std::string ToString() const {
		char buffer[16] = { 0 };
		snprintf(buffer, 16, "%d,%d", row, column);
		return buffer;
	}

private:
	friend bool operator==(const GomokuGameMove& lhs, const GomokuGameMove& rhs) noexcept;
};

inline bool operator==(const GomokuGameMove& lhs, const GomokuGameMove& rhs) noexcept {
	return lhs.row == rhs.row && lhs.column == rhs.column;
}

}

namespace std {
	template <>
	class hash<gomoku::GomokuGameMove> {
	public:
		size_t operator()(const gomoku::GomokuGameMove& move) const {
			return  move.row ^ move.column;
		}
	};
}