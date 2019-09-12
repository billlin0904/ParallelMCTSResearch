// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

namespace xiangqi {

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

}
