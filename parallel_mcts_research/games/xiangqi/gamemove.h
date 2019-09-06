// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <cstdint>
#include <utility>

namespace xiangqi {

class XiangQiGameMove {
public:
	XiangQiGameMove(int8_t row = 0, int8_t column = 0) noexcept
		: row(row)
		, column(column) {
	}

	int8_t row;
	int8_t column;

	size_t GetHash() const noexcept {
		return row ^ column;
	}

	std::string ToString() const {
		char buffer[16] = { 0 };
		snprintf(buffer, 16, "%d,%d", row, column);
		return buffer;
	}
private:
	friend bool operator==(const XiangQiGameMove& lhs, const XiangQiGameMove& rhs) noexcept;
};

inline bool operator==(const XiangQiGameMove& lhs, const XiangQiGameMove& rhs) noexcept {
	return lhs.row == rhs.row 
		&& lhs.column == rhs.column;
}

}

namespace std {
	template <>
	class hash<xiangqi::XiangQiGameMove> {
	public:
		size_t operator()(const xiangqi::XiangQiGameMove& move) const {
			return move.GetHash();
		}
	};
}
