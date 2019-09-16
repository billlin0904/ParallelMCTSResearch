// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <cassert>
#include <sstream>

namespace xiangqi {

static const int32_t MAX_MOVE = 20;

enum Colors {
	COLOR_NONE = 0,
	COLOR_RED = 1,
	COLOR_BLACK,
};

enum PiecesType {
	PIECES_NONE = 0,
	// �N�x
	PIECES_JIANG = 1,
	// ��(Ju)
	PIECES_CHE1,
	PIECES_CHE2,
	// ��
	PIECES_PAO1,
	PIECES_PAO2,
	// ��
	PIECES_MA1,
	PIECES_MA2,
	// �L
	PIECES_BING1,
	PIECES_BING2,
	PIECES_BING3,
	PIECES_BING4,
	PIECES_BING5,
	// �h
	PIECES_SHI1,
	PIECES_SHI2,
	// ��
	PIECES_XIANG1,
	PIECES_XIANG2,
	_MAX_PIECES_TYPE_,
};

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
	assert(rhs < _MAX_PIECES_TYPE_);
	lhs << pieces_types_str[rhs];
	return lhs;
}

}
