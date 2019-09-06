// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <cstdint>
#include <sstream>

#include "gamemove.h"

namespace xiangqi {

enum PiecesType {
	PIECE_NONE = 0,
	// 將軍
	PIECE_JIANG = 1,
	// 車(Ju)
	PIECE_CHE,
	// 砲
	PIECE_PAO,
	// 馬
	PIECE_MA,
	// 兵
	PIECE_BING,
	// 士
	PIECE_SHI,
	// 相
	PIECE_XIANG,
};

inline std::ostream& operator<<(std::ostream& lhs, const PiecesType& rhs) noexcept {
	static const char * pieces_types_str[] = {
		"N",
		"J",
		"C",
		"P",
		"M",
		"B",
		"S",
		"X"
	};
	lhs << pieces_types_str[rhs];
	return lhs;
}

struct Pieces {
	Pieces(PiecesType type = PiecesType::PIECE_NONE, XiangQiGameMove pos = XiangQiGameMove())
		: type(type)
		, pos(pos) {
	}

	PiecesType type;
	XiangQiGameMove pos;

	std::string ToString() const {
		std::ostringstream ostr;
		ostr << type << "=>" << pos.ToString();
		return ostr.str();
	}

	friend bool operator==(const Pieces& lhs, const Pieces& rhs) noexcept;
};

inline bool operator==(const Pieces& lhs, const Pieces& rhs) noexcept {
	return lhs.type == rhs.type && lhs.pos == rhs.pos;
}

}

namespace std {
	template <>
	class hash<xiangqi::Pieces> {
	public:
		size_t operator()(const xiangqi::Pieces& pieces) const {
			return pieces.type ^ pieces.pos.row ^ pieces.pos.column;
		}
	};
}
