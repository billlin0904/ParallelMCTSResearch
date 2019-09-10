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
	// 將軍
	PIECE_JIANG = 1,
	// 車(Ju)
	PIECE_CHE1,
	PIECE_CHE2,
	// 砲
	PIECE_PAO1,
	PIECE_PAO2,
	// 馬
	PIECE_MA1,
	PIECE_MA2,
	// 兵
	PIECE_BING1,
	PIECE_BING2,
	PIECE_BING3,
	PIECE_BING4,
	PIECE_BING5,
	// 士
	PIECE_SHI1,
	PIECE_SHI2,
	// 相
	PIECE_XIANG1,
	PIECE_XIANG2,
	_MAX_PIECE_TYPE_,
};

}
