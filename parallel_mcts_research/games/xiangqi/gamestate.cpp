#include "gamestate.h"

namespace xiangqi {

std::vector<XiangQiGameMove> Rules::MovesOnSameLine(Colors color, int8_t row, int8_t col, const BoardStates& board) {
	std::vector<XiangQiGameMove> moves;
	for (auto i = row + 1; i <= MAX_ROW; ++i) {
		XiangQiGameMove move(i, col);
		auto itr = board.find(move);
		if (itr != board.end() && (color == (*itr).second.color)) {
			break;
		}
		moves.push_back(move);
	}
	for (auto i = row - 1; i >= MIN_ROW; --i) {
		XiangQiGameMove move(i, col);
		auto itr = board.find(move);
		if (itr != board.end() && (color == (*itr).second.color)) {
			break;
		}
		moves.push_back(move);
	}
	for (auto i = col + 1; i <= MAX_COL; ++i) {
		XiangQiGameMove move(row, i);
		auto itr = board.find(move);
		if (itr != board.end() && (color == (*itr).second.color)) {
			break;
		}
		moves.push_back(move);
	}
	for (auto i = col - 1; i >= MIN_COL; --i) {
		XiangQiGameMove move(row, i);
		auto itr = board.find(move);
		if (itr != board.end() && (color == (*itr).second.color)) {
			break;
		}
		moves.push_back(move);
	}
	return moves;
}

std::vector<XiangQiGameMove> Rules::GetCheLegalMove(Colors color, int8_t row, int8_t col, const BoardStates& board) {
	return MovesOnSameLine(color, row, col, board);
}

std::vector<XiangQiGameMove> Rules::GetMaLegalMove(Colors color, int8_t row, int8_t col, const BoardStates& board) {
	std::vector<XiangQiGameMove> moves;
	if (board.find(XiangQiGameMove(row + 1, col)) == board.end()) {
		if (row + 2 <= MAX_ROW && col + 1 <= MAX_COL) {
			moves.emplace_back(row + 2, col + 1);
		}
		if (row + 2 <= MAX_ROW && col - 1 >= MIN_COL) {
			moves.emplace_back(row + 2, col - 1);
		}
	}
	if (board.find(XiangQiGameMove(row - 1, col)) == board.end()) {
		if (row - 2 >= MIN_ROW && col + 1 <= MAX_COL) {
			moves.emplace_back(row - 2, col + 1);
		}
		if (row - 2 >= MIN_ROW && col - 1 >= MIN_COL) {
			moves.emplace_back(row - 2, col - 1);
		}
	}
	if (board.find(XiangQiGameMove(row, col + 1)) == board.end()) {
		if (row + 1 <= MAX_ROW && col + 2 <= MAX_COL) {
			moves.emplace_back(row + 1, col + 2);
		}
		if (row - 1 >= MIN_ROW && col + 2 <= MAX_COL) {
			moves.emplace_back(row - 1, col + 2);
		}
	}
	if (board.find(XiangQiGameMove(row, col - 1)) == board.end()) {
		if (row + 1 <= MAX_ROW && col - 2 >= MIN_COL) {
			moves.emplace_back(row + 1, col - 2);
			moves.emplace_back(row + 1, col - 2);
		}
	}
	return moves;
}

std::vector<XiangQiGameMove> Rules::GetPaoLegalMove(Colors color, int8_t row, int8_t col, const BoardStates& board) {
	std::vector<XiangQiGameMove> moves;
	const auto inc = [](int32_t x) { return x + 1; };
	const auto dec = [](int32_t x) { return x - 1; };
	for (auto i = row + 1; i <= MAX_ROW; ++i) {
		XiangQiGameMove move(i, col);
		if (board.find(move) != board.end()) {
			if (auto next = FindFirstOpponentOnCol(color, col, i + 1, board, inc)) {
				moves.push_back(next.value());
			}
			break;
		}
		moves.push_back(move);
	}
	for (auto i = row - 1; i >= MIN_ROW; --i) {
		XiangQiGameMove move(i, col);
		if (board.find(move) != board.end()) {
			if (auto next = FindFirstOpponentOnCol(color, col, i - 1, board, inc)) {
				moves.push_back(next.value());
			}
			break;
		}
		moves.push_back(move);
	}
	for (auto i = col + 1; i <= MAX_COL; ++i) {
		XiangQiGameMove move(row, i);
		if (board.find(move) != board.end()) {
			if (auto next = FindFirstOpponentOnRow(color, row, i + 1, board, inc)) {
				moves.push_back(next.value());
			}
			break;
		}
		moves.push_back(move);
	}
	for (auto i = col - 1; i >= MIN_COL; --i) {
		XiangQiGameMove move(row, i);
		if (board.find(move) != board.end()) {
			if (auto next = FindFirstOpponentOnRow(color, row, i - 1, board, inc)) {
				moves.push_back(next.value());
			}
			break;
		}
		moves.push_back(move);
	}
	return moves;
}

std::vector<XiangQiGameMove> Rules::GetShiLegalMove(Colors color, int8_t row, int8_t col, const BoardStates& board) {
	if (row == 2 || row == 9) {
		return std::vector<XiangQiGameMove> {
			{ row - 1, col + 1 },
			{ row - 1, col - 1 },
			{ row + 1, col + 1 },
			{ row + 1, col - 1 },
		};
	}

	if (color == COLOR_BLACK) {
		return std::vector<XiangQiGameMove>{ { 9, 5 }};
	}

	return std::vector<XiangQiGameMove>{ { 2, 5 }};
}

std::vector<XiangQiGameMove> Rules::GetBingLegalMove(Colors color, int8_t row, int8_t col, const BoardStates& board) {
	auto is_beyond_river = color == COLOR_RED ? (row > 5) : (row <= 5);
	auto moves = color == COLOR_RED ?
		std::vector<XiangQiGameMove>{ { row + 1, col }} : std::vector<XiangQiGameMove>{ { row - 1, col } };
	if (is_beyond_river) {
		moves.emplace_back(row, col - 1);
		moves.emplace_back(row, col + 1);
	}
	return moves;
}

std::vector<XiangQiGameMove> Rules::GetXiangLegalMove(Colors color, int8_t row, int8_t col, const BoardStates& board) {
	std::vector<XiangQiGameMove> moves;
	auto can_move_dowward = (color == COLOR_RED || row >= 8);
	auto can_move_upward = (row <= 3 || color != COLOR_RED);
	if (can_move_dowward && board.find(row + 1, col + 1) == board.end()) {
		if (row + 2 <= MAX_ROW && col + 2 <= MAX_COL) {
			moves.emplace_back(row + 2, col + 2);
		}
	}
	if (can_move_dowward && board.find(row + 1, col - 1) == board.end()) {
		if (row + 2 <= MAX_ROW && col - 2 >= MIN_COL) {
			moves.emplace_back(row + 2, col - 2);
		}
	}
	if (can_move_dowward && board.find(row - 1, col + 1) == board.end()) {
		if (row - 2 >= MIN_ROW && col + 2 <= MAX_COL) {
			moves.emplace_back(row - 2, col + 2);
		}
	}
	if (can_move_dowward && board.find(row - 1, col - 1) == board.end()) {
		if (row - 2 >= MIN_ROW && col + 2 <= MAX_COL) {
			moves.emplace_back(row - 2, col + 2);
		}
	}
	return moves;
}

std::vector<XiangQiGameMove> Rules::GetJiangLegalMove(Colors color, int8_t row, int8_t col, const BoardStates& board) {
	std::vector<XiangQiGameMove> moves;
	for (auto c = 4; c <= 6; ++c) {
		XiangQiGameMove move(row, c);
		auto itr = board.find(move);
		if (itr != board.end() && (*itr).second.color != color) {
			moves.emplace_back(row, c);
		}
	}
	if (row < 5) {
		for (auto r = 1; r <= 3; ++r) {
			moves.emplace_back(r, col);
		}
	}
	else {
		for (auto r = 8; r <= 10; ++r) {
			moves.emplace_back(r, col);
		}
	}
	return Filter(moves, [row, col](const XiangQiGameMove& move) {
		auto r = (move.row - row) * (move.row - row);
		auto c = (move.column - col) * (move.column - col);
		return r + c >= 2;
		});
}

std::vector<XiangQiGameMove> Rules::GetPossibleMoves(const Pieces& pieces, const BoardStates& board) {
	typedef std::vector<XiangQiGameMove>(*GetLegalMoveCallback)(
		Colors color, int8_t row, int8_t col, const BoardStates & board);
	static const GetLegalMoveCallback sCallback[] = {
		{ nullptr },
		{ &GetJiangLegalMove },
		{ &GetCheLegalMove },
		{ &GetCheLegalMove },
		{ &GetPaoLegalMove },
		{ &GetPaoLegalMove },
		{ &GetMaLegalMove },
		{ &GetMaLegalMove },
		{ &GetBingLegalMove },
		{ &GetBingLegalMove },
		{ &GetBingLegalMove },
		{ &GetBingLegalMove },
		{ &GetBingLegalMove },
		{ &GetShiLegalMove },
		{ &GetShiLegalMove },
		{ &GetXiangLegalMove },
		{ &GetXiangLegalMove },
	};
	return sCallback[pieces.type](pieces.color, pieces.pos.row, pieces.pos.column, board);
}

}