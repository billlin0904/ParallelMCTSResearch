// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <cassert>
#include <cstdint>
#include <sstream>

#include "piecestype.h"
#include "gamemove.h"

namespace xiangqi {

inline std::ostream& operator<<(std::ostream& lhs, const Colors& rhs) noexcept {
	if (rhs == COLOR_BLACK) {
		lhs << "B";
	}
	else {
		lhs << "R";
	}
	return lhs;
}

inline std::ostream& operator<<(std::ostream& lhs, const PiecesType& rhs) noexcept {
	static const std::string pieces_types_str[] = {
		"N0",
		"J0",
		"C1", "C2",
		"P1", "P2",
		"M1", "M2",
		"B1", "B2", "B3", "B4", "B5",
		"S1", "S2",
		"X1", "X2"
	};
	assert(rhs < _MAX_PIECE_TYPE_);
	lhs << pieces_types_str[rhs];
	return lhs;
}

struct Pieces {
	Pieces(Colors color = COLOR_NONE, PiecesType type = PiecesType::PIECE_NONE, XiangQiGameMove pos = XiangQiGameMove())
		: color(color)
		, type(type)
		, pos(pos) {
	}

	Colors color;
	PiecesType type;
	XiangQiGameMove pos;

	friend std::ostream& operator<<(std::ostream& ostr, const Pieces& pieces) noexcept;
	friend bool operator==(const Pieces& lhs, const Pieces& rhs) noexcept;
};

inline bool operator==(const Pieces& lhs, const Pieces& rhs) noexcept {
	return lhs.color == rhs.color && lhs.type == rhs.type && lhs.pos == rhs.pos;
}

inline std::ostream& operator<<(std::ostream& ostr, const Pieces& pieces) noexcept {
	ostr << pieces.color << pieces.type << "(" << pieces.pos.ToString() << ")";
	return ostr;
}

}

namespace std {
	template <>
	class hash<xiangqi::Pieces> {
	public:
		size_t operator()(const xiangqi::Pieces& pieces) const {
			return pieces.color ^ pieces.type ^ pieces.pos.row ^ pieces.pos.column;
		}
	};
}
