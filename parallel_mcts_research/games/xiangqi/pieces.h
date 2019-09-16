// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <cassert>
#include <cstdint>
#include <sstream>

#include "piecestype.h"
#include "gamemove.h"
#include "smallvector.h"

namespace xiangqi {

using LegalMoves = SmallVector<XiangQiGameMove, MAX_MOVE>;

struct Pieces {
	Pieces(Colors color = COLOR_NONE, PiecesType type = PiecesType::PIECES_NONE, XiangQiGameMove pos = XiangQiGameMove())
		: color(color)
		, type(type)
		, pos(pos) {
	}

	~Pieces() noexcept = default;

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
