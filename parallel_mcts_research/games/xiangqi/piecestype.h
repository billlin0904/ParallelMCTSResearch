// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

namespace xiangqi {

enum Colors {
	COLOR_NONE = 0,
	COLOR_RED = 1,
	COLOR_BLACK,
};

enum PiecesType {
	PIECE_NONE = 0,
	// �N�x
	PIECE_JIANG = 1,
	// ��(Ju)
	PIECE_CHE1,
	PIECE_CHE2,
	// ��
	PIECE_PAO1,
	PIECE_PAO2,
	// ��
	PIECE_MA1,
	PIECE_MA2,
	// �L
	PIECE_BING1,
	PIECE_BING2,
	PIECE_BING3,
	PIECE_BING4,
	PIECE_BING5,
	// �h
	PIECE_SHI1,
	PIECE_SHI2,
	// ��
	PIECE_XIANG1,
	PIECE_XIANG2,
	_MAX_PIECE_TYPE_,
};

}
