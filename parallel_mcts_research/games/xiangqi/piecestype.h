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
	// 將軍
	PIECES_JIANG = 1,
	// 車(Ju)
	PIECES_CHE1,
	PIECES_CHE2,
	// 砲
	PIECES_PAO1,
	PIECES_PAO2,
	// 馬
	PIECES_MA1,
	PIECES_MA2,
	// 兵
	PIECES_BING1,
	PIECES_BING2,
	PIECES_BING3,
	PIECES_BING4,
	PIECES_BING5,
	// 士
	PIECES_SHI1,
	PIECES_SHI2,
	// 相
	PIECES_XIANG1,
	PIECES_XIANG2,
	_MAX_PIECES_TYPE_,
};

}
