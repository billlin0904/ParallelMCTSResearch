// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <cstdint>
#include <utility>

namespace xiang_qi {

struct XiangQiGameMove {
	int8_t row{ 0 };
	int8_t column{ 0 };	
};

inline bool operator==(const XiangQiGameMove& lhs, const XiangQiGameMove& rhs) noexcept {
	return lhs.row == rhs.row 
		&& lhs.column == rhs.column;
}

}

namespace std {
	template <>
	class hash<xiang_qi::XiangQiGameMove> {
	public:
		size_t operator()(const xiang_qi::XiangQiGameMove& move) const {
			return  move.row ^ move.column;
		}
	};
}
