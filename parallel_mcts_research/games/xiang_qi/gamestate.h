// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <vector>

#include "../mcts.h"
#include "gamemove.h"

namespace xiang_qi {

struct Pieces {
	bool is_red{ false };
	bool is_dead{ false };
	PiecesType pieces{ PIECE_NONE };
	XiangQiGameMove move;	
};

enum PiecesType {
	PIECE_NONE = -1,
	PIECE_JIANG = 0,
	PIECE_CHE,
	PIECE_PAO,
	PIECE_MA,
	PIECE_BING,
	PIECE_SHI,
	PIECE_XIANG,
};

static Pieces GetPiecesFromID(int32_t id) {
	static const Pieces sPieces[16] = {
		{false, false, PiecesType::PIECE_CHE,   0, 0},
		{false, false, PiecesType::PIECE_MA,    0, 1},
		{false, false, PiecesType::PIECE_XIANG, 0, 2},
		{false, false, PiecesType::PIECE_SHI,   0, 3},
		{false, false, PiecesType::PIECE_JIANG, 0, 4},
		{false, false, PiecesType::PIECE_SHI,   0, 5},
		{false, false, PiecesType::PIECE_XIANG, 0, 6},
		{false, false, PiecesType::PIECE_MA,    0, 7},
		{false, false, PiecesType::PIECE_CHE,   0, 8},
			    
		{false, false, PiecesType::PIECE_PAO , 2, 1 },
		{false, false, PiecesType::PIECE_PAO , 2, 7 },
		{false, false, PiecesType::PIECE_BING, 3, 0 },
		{false, false, PiecesType::PIECE_BING, 3, 2 },
		{false, false, PiecesType::PIECE_BING, 3, 4 },
		{false, false, PiecesType::PIECE_BING, 3, 6 },
		{false, false, PiecesType::PIECE_BING, 3, 8 },
	};

	if (id < 16) {
		return sPieces[id];
	}

	return Pieces{
		false,
		true,
		sPieces[id - 16].pieces,
		{9 - sPieces[id - 16].move.row, 8 - sPieces[id - 16].move.column},
	};
}

class XiangQiGameState {
public:
	XiangQiGameState()
		: winner_exists_(false)
		, is_terminal_(false)
		, player_id_(1) {
		for (auto i = 0; i < 32; ++i) {
			pieces_.push_back(GetPiecesFromID(i));
		}
	}

	void ApplyMove(const XiangQiGameMove& move) {
	}

	double Evaluate() const noexcept {
		return 0.0;
	}

	XiangQiGameMove GetRandomMove() const {
		
	}

	int8_t GetPlayerID() const noexcept {
		return player_id_;
	}

	bool IsLegalMove(const XiangQiGameMove& move) const {

	}

	HashSet<XiangQiGameMove> GetLegalMoves() const noexcept {
	}

	std::string ToString() const {
		return "";
	}

	int8_t GetWinner() const {
		return player_id_;
	}
private:
	int32_t GetPiecesID(int32_t row, int32_t col) const {
		int32_t id = 0;
		for (auto pieces : pieces_) {
			if (pieces.move.row == row && pieces.move.column == col && !pieces.is_dead) {
				return id;
			}
			++id;
		}
		return -1;
	}

	int32_t GetMaxStep(int32_t id, const XiangQiGameMove& move) const {
		return abs(move.row - pieces_[id].move.row) * 10 + abs(move.column - pieces_[id].move.column);
	}

	bool CanMove(int32_t id, int32_t kill_id, const XiangQiGameMove &move) {
		if (pieces_[id].is_red == pieces_[kill_id].is_red) {
			return false;
		}

		switch (pieces_[id].pieces) {
		case PiecesType::PIECE_JIANG:
			break;
		}
	}

	bool CanMoveJiang(int32_t id, int32_t kill_id, const XiangQiGameMove& move) const {
		if (pieces_[id].is_red) {
			if (move.row < 7) {
				return false;
			}
		}
		else {
			if (move.row > 2) {
				return false;
			}
		}

		if (move.column < 3 || move.column > 5) {
			return false;
		}

		auto max_step = GetMaxStep(id, move);
		if (max_step == 10 || max_step == 1) {
			return true;
		}
		return false;
	}	

	bool CanMoveShi(int32_t id, int32_t kill_id, const XiangQiGameMove& move) const {
		if (pieces_[id].is_red) {
			if (move.row < 7) {
				return false;
			}
		}
		else {
			if (move.row > 2) {
				return false;
			}
		}

		if (move.column < 3 || move.column > 5) {
			return false;
		}
		auto max_step = GetMaxStep(id, move);
		if (max_step == 11) {
			return true;
		}
		return false;
	}

	bool CanMoveXiang(int32_t id, int32_t kill_id, const XiangQiGameMove& move) const {
		if (pieces_[id].is_red) {
			if (move.row < 5) {
				return false;
			}
		}
		else {
			if (move.row > 4) {
				return false;
			}
		}

		auto row_eye = (move.row + pieces_[id].move.row) / 2;
		auto col_eye = (move.column + pieces_[id].move.column) / 2;
		if (GetPiecesID(row_eye, col_eye) != -1) {
			return false;
		}
		auto max_step = GetMaxStep(id, move);
		if (max_step == 22) {
			return true;
		}
		return false;
	}	

	bool winner_exists_;
	bool is_terminal_;
	int8_t player_id_;
	std::vector<Pieces> pieces_;
};

}
