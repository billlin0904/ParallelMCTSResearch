#include "gamestate.h"

namespace xiangqi {

static inline bool IsBeyonRiver(Colors color, int8_t row) noexcept {
	return color == COLOR_RED ? (row > 5) : (row <= 5);
}

static inline bool IsExistPieces(int8_t row, int8_t col, const BoardStates& board) noexcept {
	return board.find(XiangQiGameMove(row, col)) != board.end();
}

static inline bool IsLegalMove(Colors color, int8_t row, int8_t col, const BoardStates& board) noexcept {
	auto itr = board.find(XiangQiGameMove(row, col));
	if (itr == board.end()) {
		// 棋盤該位置為空該位置可以移動至
		return true;
	}
	// 棋盤該該位置不為空, 且不是相同顏色的棋子
	return (*itr).second.color != color;
}

static inline bool IsLegalMove(Colors color, const XiangQiGameMove move, const BoardStates& board) noexcept {
	return IsLegalMove(color, move.row, move.column, board);
}

static inline bool IsLegalMove(const Pieces pieces, const BoardStates& board) noexcept {
	return IsLegalMove(pieces.color, pieces.pos, board);
}

template <typename Function>
static inline std::optional<XiangQiGameMove> FindFirstOpponentOnCol(
	Colors color, int8_t col, int8_t start_row, const BoardStates& board, Function&& f) {
	for (; start_row >= MIN_ROW && start_row <= MAX_ROW; start_row = f(start_row)) {
		XiangQiGameMove move(start_row, col);
		if (IsLegalMove(color, move, board)) {
			return move;
		}
		else {
			break;
		}
	}
	return std::nullopt;
}

template <typename Function>
static inline std::optional<XiangQiGameMove> FindFirstOpponentOnRow(
	Colors color, int8_t row, int8_t start_col, const BoardStates& board, Function&& f) {
	for (; start_col >= MIN_COL && start_col <= MAX_COL; start_col = f(start_col)) {
		XiangQiGameMove move(row, start_col);
		if (IsLegalMove(color, move, board)) {
			return move;
		}
		else {
			break;
		}
	}
	return std::nullopt;
}

const std::vector<Pieces>& Rules::GetRedPieces() {
	static const std::vector<Pieces> pieces{
		{ COLOR_RED, PIECES_CHE1,   {1, 1} },
		{ COLOR_RED, PIECES_CHE2,   {1, 9} },
		{ COLOR_RED, PIECES_PAO1,   {3, 2} },
		{ COLOR_RED, PIECES_PAO2,   {3, 8} },
		{ COLOR_RED, PIECES_MA1,    {1, 2} },
		{ COLOR_RED, PIECES_MA2,    {1, 8} },
		{ COLOR_RED, PIECES_XIANG1, {1, 3} },
		{ COLOR_RED, PIECES_XIANG2, {1, 7} },
		{ COLOR_RED, PIECES_SHI1,   {1, 4} },
		{ COLOR_RED, PIECES_SHI2,   {1, 6} },
		{ COLOR_RED, PIECES_BING1,  {4, 1} },
		{ COLOR_RED, PIECES_BING2,  {4, 3} },
		{ COLOR_RED, PIECES_BING3,  {4, 5} },
		{ COLOR_RED, PIECES_BING4,  {4, 7} },
		{ COLOR_RED, PIECES_BING5,  {4, 9} },
		{ COLOR_RED, PIECES_JIANG,  {1, 5} },
	};
	return pieces;
}

const std::vector<Pieces>& Rules::GetBlackPieces() {
	static const std::vector<Pieces> pieces{
		{ COLOR_BLACK, PIECES_CHE1,   {10, 1} },
		{ COLOR_BLACK, PIECES_CHE2,   {10, 9} },
		{ COLOR_BLACK, PIECES_PAO1,   {8,  2} },
		{ COLOR_BLACK, PIECES_PAO2,   {8,  8} },
		{ COLOR_BLACK, PIECES_MA1,    {10, 2} },
		{ COLOR_BLACK, PIECES_MA2,    {10, 8} },
		{ COLOR_BLACK, PIECES_XIANG1, {10, 3} },
		{ COLOR_BLACK, PIECES_XIANG2, {10, 7} },
		{ COLOR_BLACK, PIECES_SHI1,   {10, 4} },
		{ COLOR_BLACK, PIECES_SHI2,   {10, 6} },
		{ COLOR_BLACK, PIECES_BING1,  {7,  1} },
		{ COLOR_BLACK, PIECES_BING2,  {7,  3} },
		{ COLOR_BLACK, PIECES_BING3,  {7,  5} },
		{ COLOR_BLACK, PIECES_BING4,  {7,  7} },
		{ COLOR_BLACK, PIECES_BING5,  {7,  9} },
		{ COLOR_BLACK, PIECES_JIANG,  {10, 5} },		
	};
	return pieces;
}

static int32_t GetRowPiecesCount(Colors color, int8_t row, int8_t col, const BoardStates& board) {
	int32_t count = 0;
	if (color == COLOR_BLACK) {
		for (auto i = row - 1; i >= MIN_ROW; --i) {
			XiangQiGameMove move(i, col);
			auto itr = board.find(move);
			if (itr != board.end()) {
				++count;
			}			
		}
	}
	else {
		for (auto i = row + 1; i <= MAX_ROW; ++i) {
			XiangQiGameMove move(i, col);
			auto itr = board.find(move);
			if (itr != board.end()) {
				++count;
			}
		}
	}
	return count;
}

static LegalMoves MovesOnSameLine(Colors color, int8_t row, int8_t col, const BoardStates& board) {
	LegalMoves moves;
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

LegalMoves GetCheLegalMove(Colors color, int8_t row, int8_t col, const BoardStates& board) {
	return MovesOnSameLine(color, row, col, board);
}

LegalMoves GetMaLegalMove(Colors color, int8_t row, int8_t col, const BoardStates& board) {
	LegalMoves moves;
	if (board.find(XiangQiGameMove(row + 1, col)) == board.end()) {
		if (row + 2 <= MAX_ROW && col + 1 <= MAX_COL) {
			if (IsLegalMove(color, row + 2, col + 1, board)) {
				moves.emplace_back(row + 2, col + 1);
			}			
		}
		if (row + 2 <= MAX_ROW && col - 1 >= MIN_COL) {
			if (IsLegalMove(color, row + 2, col - 1, board)) {
				moves.emplace_back(row + 2, col - 1);
			}
		}
	}
	if (board.find(XiangQiGameMove(row - 1, col)) == board.end()) {
		if (row - 2 >= MIN_ROW && col + 1 <= MAX_COL) {
			if (IsLegalMove(color, row - 2, col + 1, board)) {
				moves.emplace_back(row - 2, col + 1);
			}
		}
		if (row - 2 >= MIN_ROW && col - 1 >= MIN_COL) {
			if (IsLegalMove(color, row - 2, col - 1, board)) {
				moves.emplace_back(row - 2, col - 1);
			}
		}
	}
	if (board.find(XiangQiGameMove(row, col + 1)) == board.end()) {
		if (row + 1 <= MAX_ROW && col + 2 <= MAX_COL) {
			if (IsLegalMove(color, row + 1, col + 2, board)) {
				moves.emplace_back(row + 1, col + 2);
			}
		}
		if (row - 1 >= MIN_ROW && col + 2 <= MAX_COL) {
			if (IsLegalMove(color, row - 1, col + 2, board)) {
				moves.emplace_back(row - 1, col + 2);
			}
		}
	}
	if (board.find(XiangQiGameMove(row, col - 1)) == board.end()) {
		if (row + 1 <= MAX_ROW && col - 2 >= MIN_COL) {
			if (IsLegalMove(color, row + 1, col - 2, board)) {
				moves.emplace_back(row + 1, col - 2);				
			}
			if (IsLegalMove(color, row - 1, col - 2, board)) {
				moves.emplace_back(row - 1, col - 2);
			}
		}
	}
	return moves;
}

LegalMoves GetPaoLegalMove(Colors color, int8_t row, int8_t col, const BoardStates& board) {
	LegalMoves moves;
	const auto inc = [](int32_t x) noexcept { return x + 1; };
	const auto dec = [](int32_t x) noexcept { return x - 1; };
	for (auto i = row + 1; i <= MAX_ROW; ++i) {
		XiangQiGameMove move(i, col);
		// 該下一個棋子存在
		if (board.find(move) != board.end()) {
			// 尋找再下一個棋子是對手的棋子
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

LegalMoves GetBingLegalMove(Colors color, int8_t row, int8_t col, const BoardStates& board) {
	auto is_beyond_river = IsBeyonRiver(color, row);
	LegalMoves moves;
	if (color == COLOR_RED) {
		if (row + 1 <= MAX_ROW) {
			if (IsLegalMove(color, row + 1, col, board)) {
				moves.emplace_back(row + 1, col);
			}
		}
	}
	else {
		if (row - 1 >= MIN_ROW) {
			if (IsLegalMove(color, row - 1, col, board)) {
				moves.emplace_back(row - 1, col);
			}			
		}
	}
	if (is_beyond_river) {
		if (col - 1 >= MIN_COL) {
			if (IsLegalMove(color, row, col - 1, board)) {
				moves.emplace_back(row, col - 1);
			}
		}
		if (col + 1 <= MAX_COL) {
			if (IsLegalMove(color, row, col + 1, board)) {
				moves.emplace_back(row, col + 1);
			}
		}
	}
	return moves;
}

LegalMoves GetXiangLegalMove(Colors color, int8_t row, int8_t col, const BoardStates& board) {
	LegalMoves moves;
	auto can_move_dowward = (color == COLOR_RED || row >= 8);
	auto can_move_upward = (row <= 3 || color != COLOR_RED);

	if (can_move_upward && board.find(XiangQiGameMove(row + 1, col + 1)) == board.end()) {
		if (row + 2 <= MAX_ROW && col + 2 <= MAX_COL) {
			if (!IsBeyonRiver(color, row + 2)) {
				XiangQiGameMove move(row + 2, col + 2);
				if (IsLegalMove(color, move, board)) {
					moves.push_back(move);
				}				
			}
		}
	}
	if (can_move_upward && board.find(XiangQiGameMove(row + 1, col - 1)) == board.end()) {
		if (row + 2 <= MAX_ROW && col - 2 >= MIN_COL) {
			if (!IsBeyonRiver(color, row + 2)) {
				XiangQiGameMove move(row + 2, col - 2);
				if (IsLegalMove(color, move, board)) {
					moves.push_back(move);
				}
			}			
		}
	}
	if (can_move_dowward && board.find(XiangQiGameMove(row - 1, col + 1)) == board.end()) {
		if (row - 2 >= MIN_ROW && col + 2 <= MAX_COL) {			
			if (!IsBeyonRiver(color, row - 2)) {
				XiangQiGameMove move(row - 2, col + 2);
				if (IsLegalMove(color, move, board)) {
					moves.emplace_back(move);
				}				
			}
		}
	}
	if (can_move_dowward && board.find(XiangQiGameMove(row - 1, col - 1)) == board.end()) {
		if (row - 2 >= MIN_ROW && col - 2 >= MIN_COL) {
			if (!IsBeyonRiver(color, row - 2)) {
				XiangQiGameMove move(row - 2, col - 2);
				if (IsLegalMove(color, move, board)) {
					moves.emplace_back(row - 2, col - 2);
				}				
			}
		}
	}
	return moves;
}

LegalMoves GetShiLegalMove(Colors color, int8_t row, int8_t col, const BoardStates& board) {
	const LegalMoves legal_moves {
		{ row - 1, col + 1 },
		{ row - 1, col - 1 },
		{ row + 1, col + 1 },
		{ row + 1, col - 1 },
	};

	LegalMoves moves;

	if (row == 2 || row == 9) {		
		for (auto move : legal_moves) {
			if (move.row < MIN_ROW || move.row > MAX_ROW) {
				continue;
			}
			if (move.column < MIN_COL || move.column > MAX_COL) {
				continue;
			}
			if (IsLegalMove(color, move, board)) {
				moves.push_back(move);
			}
		}
		return moves;
	}

	if (color == COLOR_BLACK) {
		if (IsLegalMove(color, 9, 5, board)) {
			moves.emplace_back(9, 5);
		}
	}
	else {
		if (IsLegalMove(color, 2, 5, board)) {
			moves.emplace_back(2, 5);
		}
	}
	return moves;
}

LegalMoves GetJiangLegalMove(Colors color, int8_t row, int8_t col, const BoardStates& board) {
	LegalMoves moves;
	auto found_opp_jiang = false;
	auto pieces_count = 0;

	if (color == COLOR_BLACK) {		
		for (auto i = row - 1; i >= MIN_ROW; --i) {
			XiangQiGameMove move(i, col);
			auto itr = board.find(move);
			if (itr != board.end()) {
				if (!found_opp_jiang) {
					if ((*itr).second.type != PIECES_JIANG) {
						++pieces_count;
					}
					found_opp_jiang = (*itr).second.type == PIECES_JIANG;
					if (pieces_count == 0) {
						//std::cout << board << "\n";
						break;
					}
				}
			}
		}
	}
	else {
		for (auto i = row + 1; i <= MAX_ROW; ++i) {
			XiangQiGameMove move(i, col);
			auto itr = board.find(move);
			if (itr != board.end()) {
				if (!found_opp_jiang) {
					if ((*itr).second.type != PIECES_JIANG) {
						++pieces_count;
					}
					found_opp_jiang = (*itr).second.type == PIECES_JIANG;
					if (pieces_count == 0) {
						//std::cout << board << "\n";						
						break;
					}
				}
			}
		}
	}

	if (col + 1 <= 6) {
		XiangQiGameMove move(row, col + 1);
		if (IsLegalMove(color, move, board)) {
			moves.emplace_back(move);
		}
	}

	if (col - 1 >= 4) {
		XiangQiGameMove move(row, col - 1);
		if (IsLegalMove(color, move, board)) {
			moves.emplace_back(move);
		}
	}

	if (!found_opp_jiang || pieces_count != 0) {
		if (row < 5) {
			for (auto r = row + 1; r <= 3; ++r) {
				XiangQiGameMove move(r, col);
				if (IsLegalMove(color, move, board)) {
					moves.emplace_back(move);
				}
				break;
			}

			auto r = row - 1;
			if (r > 0) {
				for (; r <= MIN_ROW; --r) {
					XiangQiGameMove move(r, col);
					if (IsLegalMove(color, move, board)) {
						moves.emplace_back(move);
					}
					break;
				}
			}
		}
		else {
			auto r = row - 1;
			for (; r >= 8; --r) {
				XiangQiGameMove move(r, col);
				if (IsLegalMove(color, move, board)) {
					moves.emplace_back(move);
				}
				break;
			}

			for (r = row + 1; r <= MAX_ROW; ++r) {
				XiangQiGameMove move(r, col);
				if (IsLegalMove(color, move, board)) {
					moves.emplace_back(move);
				}
				break;
			}
		}
	}	
	
	return moves;
}

bool Rules::IsCaptureOppJiang(const Pieces& pieces, const BoardStates& board) {
	Pieces opp_pieces;
	int32_t count = 0;
	if (pieces.color == COLOR_BLACK) {
		for (auto i = pieces.pos.row - 1; i >= MIN_ROW; --i) {
			XiangQiGameMove move(i, pieces.pos.column);
			auto itr = board.find(move);
			if (itr != board.end()) {
				if ((*itr).second.type != PIECES_JIANG) {
					count++;
				}
				else {
					opp_pieces = (*itr).second;
				}
			}
		}
	}
	else {
		for (auto i = pieces.pos.row + 1; i <= MAX_ROW; ++i) {
			XiangQiGameMove move(i, pieces.pos.column);
			auto itr = board.find(move);
			if (itr != board.end()) {
				if ((*itr).second.type != PIECES_JIANG) {
					count++;
				}
				else {
					opp_pieces = (*itr).second;
				}
			}
		}
	}
	return count == 0 && opp_pieces.type == PIECES_JIANG;
}

LegalMoves Rules::GetPossibleMoves(const Pieces& pieces, const BoardStates& board) {
	typedef LegalMoves(*GetLegalMoveCallback)(
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
	assert(pieces.type < _MAX_PIECES_TYPE_);
	return sCallback[pieces.type](pieces.color, pieces.pos.row, pieces.pos.column, board);
}

}