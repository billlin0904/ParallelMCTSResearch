// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <cstdint>
#include <utility>

namespace gomoku {

struct GomokuGameMove {
	explicit GomokuGameMove(int8_t row = 0, int8_t column = 0) noexcept
		: row(row)
		, column(column) {
	}
	
	int8_t row;
	int8_t column;

private:
	friend std::ostream& operator<<(std::ostream& ostr, const GomokuGameMove &move);
	friend bool operator==(const GomokuGameMove& lhs, const GomokuGameMove& rhs) noexcept;
};

inline bool operator==(const GomokuGameMove& lhs, const GomokuGameMove& rhs) noexcept {
	return lhs.row == rhs.row && lhs.column == rhs.column;
}

inline std::ostream& operator<<(std::ostream& ostr, const GomokuGameMove& move) {
	ostr << static_cast<int32_t>(move.row) << "," << static_cast<int32_t>(move.column);
	return ostr;
}

}

template <typename T>
void HashCombine(std::size_t& seed, const T& v) noexcept {
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

namespace std {
	template <>
	struct hash<gomoku::GomokuGameMove> {
		size_t operator()(const gomoku::GomokuGameMove& move) const noexcept {
			size_t seed = 0;
			::HashCombine(seed, move.row);
			::HashCombine(seed, move.column);
			return seed;
		}
	};
}