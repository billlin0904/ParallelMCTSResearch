#include <array>
#include <map>
#include <iostream>
#include <cassert>
#include <fstream>
#include <sstream>

#include "mcts.h"

class GameMove {
public:
    GameMove(size_t index = 0) noexcept
        : index(index) {
    }

    size_t index;

	std::string ToString() const {
		std::ostringstream ostr;
		ostr << index;
		return ostr.str();
	}

private:	
    friend bool operator==(const GameMove& lhs, const GameMove& rhs) noexcept;
};

bool operator==(const GameMove& lhs, const GameMove& rhs) noexcept {
    return lhs.index == rhs.index;
}

using namespace mcts;

static const int8_t PLAYER1 = 'O';
static const int8_t PLAYER2 = 'X';
static const int8_t EMPTY = ' ';

class GameState {
public:
    GameState() noexcept
        : winner_exists_(false)
        , is_terminal_(false)
        , player_id_(1) {
        board_.fill(EMPTY);
    }

    bool IsTerminal() const noexcept {
        return is_terminal_;
    }

    bool IsWinnerExsts() const noexcept {
        return winner_exists_;
    }

    void ApplyMove(const GameMove &move) {
        if (player_id_ == 1) {
            board_.at(move.index) = PLAYER1;
        }
        else {
            board_.at(move.index) = PLAYER2;
        }

        if (CheckWinner() != EMPTY) {
            winner_exists_ = true;
            is_terminal_ = true;
        } else {
            auto itr = std::find(board_.begin(), board_.end(), EMPTY);
            if (itr == board_.end()) {
                is_terminal_ = true;
            }
        }
        player_id_ = ((player_id_ == 1) ? 2 : 1);
        last_move_ = move;
    }

    double Evaluate() const noexcept {
        if ((is_terminal_ == true) && (winner_exists_ == false)) {
            return 0.0;
        }
        return (player_id_ == 1) ? -1 : 1;
    }

    GameMove GetRandomMove() const {
        auto legal_moves = GetLegalMoves();
        return legal_moves.at(RNG::Get()(0, int32_t(legal_moves.size() - 1)));
    }

    int8_t GetPlayerID() const noexcept {
        return player_id_;
    }

    bool IsLegalMove(const GameMove &move) const {
        if (board_.size() < move.index) {
            return false;
        }
        return board_.at(move.index) == EMPTY;
    }

    std::vector<GameMove> GetLegalMoves() const noexcept {
        std::vector<GameMove> legal_moves;
        int32_t i = 0;
        for (auto c : board_) {
            if (c == EMPTY) {
                legal_moves.emplace_back(i);
            }
            i++;
        }
        return legal_moves;
    }

    int8_t CheckWinner() const noexcept {
        for (auto i = 0; i < 9; i += 3) {
            if (board_[i] == board_[i + 1] && board_[i + 1] == board_[i + 2] && board_[i]) {
                return board_[i];
            }
        }

        for (auto i = 0; i < 3; i++) {
            if (board_[i] == board_[i + 3] && board_[i + 3] == board_[i + 6] && board_[i]) {
                return board_[i];
            }
        }

        if (board_[0] == board_[4] && board_[4] == board_[8] && board_[0]) {
            return board_[0];
        }

        if (board_[2] == board_[4] && board_[4] == board_[6] && board_[2]) {
            return board_[2];
        }

        return EMPTY;
    }

	std::string ToString() const {
		std::ostringstream ostr;
		for (auto i = 0; i < 3; ++i) {
			auto idx = 3 * i;
			ostr << board_[idx] << board_[idx + 1] << board_[idx + 2];
		}
		return ostr.str();
	}

private:
    friend std::ostream& operator<<(std::ostream& ostr, const GameState& state) {
        for (auto i = 0; i < 3; ++i) {
            auto idx = 3 * i;
            ostr << state.board_[idx] << " | " << state.board_[idx + 1] << " | " << state.board_[idx + 2] << '\n';
            std::cout << "---------" << '\n';
        }
        return ostr;
    }

    bool winner_exists_;
    bool is_terminal_;
    int8_t player_id_;
    GameMove last_move_;
    std::array<int8_t, 9> board_;
};

int main() {	
    std::map<int8_t, size_t> stats;

    while (true) {
        MCTS<GameState, GameMove> ai1;
        MCTS<GameState, GameMove> ai2;
        GameState game;

        while (!game.IsTerminal()) {
            if (game.GetPlayerID() == 1) {
				/*
                std::cout << "Human turn!" << "\n";
				std::cout << game;

                while (true) {
                    int32_t input = 0;
                    std::cin >> input;
                    if (game.IsLegalMove(input)) {
                        GameMove human_move(input);
                        game.ApplyMove(human_move);
						ai1.SetOpponentMove(human_move);
                        break;
                    }
                }
				*/
				
                auto move = ai2.Search(100);
                assert(game.IsLegalMove(move));
                //std::cout << "AI2 turn!" << "\n";
                game.ApplyMove(move);
                ai1.SetOpponentMove(move);
            } else {
                auto move = ai1.Search(100);
                assert(game.IsLegalMove(move));
                //std::cout << "AI1 turn!" << "\n";
                game.ApplyMove(move);
                ai2.SetOpponentMove(move);
								
				std::cout << ai2 << "\n";
            }

            std::cout << game;
			std::cin.get();
        }

        std::cout << game;

        if (game.IsWinnerExsts()) {
            stats[game.CheckWinner()]++;
        } else {
            stats[EMPTY]++;
        }

        //std::cout << "Winner is:" << game.CheckWinner() << "\n";
        std::cout << PLAYER1 << " win:" << stats[PLAYER1] << "\n";
        std::cout << PLAYER2 << " win:" << stats[PLAYER2] << "\n";
        std::cout << "Tie" << " win:" << stats[EMPTY] << "\n";
    }
}
