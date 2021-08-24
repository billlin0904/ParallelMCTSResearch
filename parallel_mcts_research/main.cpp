// Copyright (c) 2019 ParallelMCTSResearch project.

#include <iostream>
#include <map>

#include "mcts.h"
#include "games/tictactoe/gamestate.h"
#include "games/gomoku/gamestate.h"

using namespace mcts;

template <typename State, typename Move>
std::map<int8_t, size_t> Simulation(int32_t count, bool is_show_game) {
	std::map<int8_t, size_t> stats;
	
	for (auto i = 0; i < count; ++i) {
		MCTS<State, Move> ai1(50000, 100000);
		MCTS<State, Move> ai2(50000, 100000);

		State game;

		if (is_show_game) {
			std::cout << "Game start!" << std::endl;
		}

		while (!game.IsTerminal()) {
			if (game.GetPlayerID() == kOpponentID) {
                auto move = ai2.ParallelSearch();
				assert(game.IsLegalMove(move));
				if (is_show_game) {
					std::cout << "AI2 turn! " << move << " rate:" << static_cast<int32_t>(ai2.GetCurrentNode()->GetWinRate() * 100) << "%\n";
				}
				game.ApplyMove(move);
				ai1.SetOpponentMove(move);
			}
			else {
                auto move = ai1.ParallelSearch();
				assert(game.IsLegalMove(move));
				if (is_show_game) {
					std::cout << "AI1 turn! " << move << " rate:" << static_cast<int32_t>(ai1.GetCurrentNode()->GetWinRate() * 100) << "%\n";
				}
				game.ApplyMove(move);
				ai2.SetOpponentMove(move);
			}
			if (is_show_game) {
				std::cout << game;
			}
		}

		if (game.IsWinnerExist()) {
			++stats[game.GetWinner()];
		}
		else {
            ++stats[State::kEmpty];
		}
	}
	return stats;
}

template <typename State, typename Move>
void Eval(int32_t count = 1000, bool is_show_game = false) {
	std::map<int8_t, size_t> stats = Simulation<State, Move>(count, is_show_game);
	std::cout << State::kPlayer1 << " win:" << stats[State::kPlayer1] << "\n";
	std::cout << State::kPlayer2 << " win:" << stats[State::kPlayer2] << "\n";
	std::cout << "Tie" << " win:" << stats[State::kEmpty] << "\n";
}

int main() {
    using namespace gomoku;
    Eval<GomokuGameState, GomokuGameMove>(1000, true);
    //using namespace tictactoe;
    //Eval<TicTacToeGameState, TicTacToeGameMove>(1000, true);
	std::cin.get();
}
