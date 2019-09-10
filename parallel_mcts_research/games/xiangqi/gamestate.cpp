#include "gamestate.h"

namespace xiangqi {

const std::vector<Pieces>& GetRedPieces() {
	static const std::vector<Pieces> pieces{
		{ Colors::COLOR_RED, PiecesType::PIECE_CHE1,   {1, 1} },
		{ Colors::COLOR_RED, PiecesType::PIECE_CHE2,   {1, 9} },
		{ Colors::COLOR_RED, PiecesType::PIECE_PAO1,   {3, 2} },
		{ Colors::COLOR_RED, PiecesType::PIECE_PAO2,   {3, 8} },
		{ Colors::COLOR_RED, PiecesType::PIECE_MA1,    {1, 2} },
		{ Colors::COLOR_RED, PiecesType::PIECE_MA2,    {1, 8} },
		{ Colors::COLOR_RED, PiecesType::PIECE_XIANG1, {1, 3} },
		{ Colors::COLOR_RED, PiecesType::PIECE_XIANG2, {1, 7} },
		{ Colors::COLOR_RED, PiecesType::PIECE_SHI1,   {1, 4} },
		{ Colors::COLOR_RED, PiecesType::PIECE_SHI2,   {1, 6} },
		{ Colors::COLOR_RED, PiecesType::PIECE_BING1,  {4, 1} },
		{ Colors::COLOR_RED, PiecesType::PIECE_BING2,  {4, 3} },
		{ Colors::COLOR_RED, PiecesType::PIECE_BING3,  {4, 5} },
		{ Colors::COLOR_RED, PiecesType::PIECE_BING4,  {4, 7} },
		{ Colors::COLOR_RED, PiecesType::PIECE_BING5,  {4, 9} },
		{ Colors::COLOR_RED, PiecesType::PIECE_JIANG,  {1, 5} },
	};
	return pieces;
}

 const std::vector<Pieces>& GetBlackPieces() {
	static const std::vector<Pieces> pieces{
		{ Colors::COLOR_BLACK, PiecesType::PIECE_CHE1,   {10, 1} },
		{ Colors::COLOR_BLACK, PiecesType::PIECE_CHE2,   {10, 9} },
		{ Colors::COLOR_BLACK, PiecesType::PIECE_PAO1,   {8,  2} },
		{ Colors::COLOR_BLACK, PiecesType::PIECE_PAO2,   {8,  8} },
		{ Colors::COLOR_BLACK, PiecesType::PIECE_MA1,    {10, 2} },
		{ Colors::COLOR_BLACK, PiecesType::PIECE_MA2,    {10, 8} },
		{ Colors::COLOR_BLACK, PiecesType::PIECE_XIANG1, {10, 3} },
		{ Colors::COLOR_BLACK, PiecesType::PIECE_XIANG2, {10, 7} },
		{ Colors::COLOR_BLACK, PiecesType::PIECE_SHI1,   {10, 4} },
		{ Colors::COLOR_BLACK, PiecesType::PIECE_SHI2,   {10, 6} },
		{ Colors::COLOR_BLACK, PiecesType::PIECE_BING1,  {7,  1} },
		{ Colors::COLOR_BLACK, PiecesType::PIECE_BING2,  {7,  3} },
		{ Colors::COLOR_BLACK, PiecesType::PIECE_BING3,  {7,  5} },
		{ Colors::COLOR_BLACK, PiecesType::PIECE_BING4,  {7,  7} },
		{ Colors::COLOR_BLACK, PiecesType::PIECE_BING5,  {7,  9} },
		{ Colors::COLOR_BLACK, PiecesType::PIECE_JIANG,  {10, 5} },
	};
	return pieces;
}

template <typename Function>
static std::optional<XiangQiGameMove> FindFirstOpponentOnCol(
	Colors color, int8_t col, int8_t start_row, const BoardStates& board, Function&& f) {
	for (; start_row >= MIN_ROW && start_row <= MAX_ROW; start_row = f(start_row)) {
		XiangQiGameMove move(start_row, col);
		auto itr = board.find(move);
		if (itr != board.end()) {
			if (color == (*itr).second.color) {
				break;
			}
			else {
				return move;
			}
		}
	}
	return std::nullopt;
}

template <typename Function>
static std::optional<XiangQiGameMove> FindFirstOpponentOnRow(
	Colors color, int8_t row, int8_t start_col, const BoardStates& board, Function&& f) {
	for (; start_col >= MIN_COL && start_col <= MAX_COL; start_col = f(start_col)) {
		XiangQiGameMove move(row, start_col);
		auto itr = board.find(move);
		if (itr != board.end()) {
			if (color == (*itr).second.color) {
				break;
			}
			else {
				return move;
			}
		}
	}
	return std::nullopt;
}

static bool IsBeyonRiver(Colors color, int8_t row, int8_t col) {
	return color == COLOR_RED ? (row > 5) : (row <= 5);
}

static Colors GetOppColor(Colors color) {
	return color == COLOR_RED ? COLOR_BLACK : COLOR_RED;
}

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
		return std::vector<XiangQiGameMove>{{ 9, 5 }};
	}

	return std::vector<XiangQiGameMove>{{ 2, 5 }};
}

std::vector<XiangQiGameMove> Rules::GetBingLegalMove(Colors color, int8_t row, int8_t col, const BoardStates& board) {
	auto is_beyond_river = IsBeyonRiver(color, row, col);
	std::vector<XiangQiGameMove> moves;
	if (color == COLOR_RED) {
		if (row + 1 <= MAX_ROW) {
			moves.emplace_back(row + 1, col);
		}		
	}
	else {
		if (row - 1 >= MIN_ROW) {
			moves.emplace_back(row - 1, col);
		}
	}
	if (is_beyond_river) {
		if (col - 1 >= MIN_COL) {
			moves.emplace_back(row, col - 1);
		}
		if (col + 1 <= MAX_COL) {
			moves.emplace_back(row, col + 1);
		}
	}
	return moves;
}

std::vector<XiangQiGameMove> Rules::GetXiangLegalMove(Colors color, int8_t row, int8_t col, const BoardStates& board) {	
	std::vector<XiangQiGameMove> moves;

	if (auto is_beyond_river = IsBeyonRiver(color, row, col)) {
		return moves;
	}

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

	bool found_opp_jiang = false;
	auto oppcolor = GetOppColor(color);
	XiangQiGameMove opp_jiang_pos(row, col);
	auto itr = board.find(opp_jiang_pos);
	if ((*itr).second.type == PIECE_JIANG) {
		found_opp_jiang = true;
	}

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
	static const GetLegalMoveCallback const sCallback[] = {
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