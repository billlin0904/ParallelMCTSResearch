// Copyright (c) 2019 ParallelMCTSResearch project.

#include <iostream>
#include <map>

#include "mcts.h"
#include "games/tictactoe/gamestate.h"
#include "games/gomoku/gamestate.h"

using namespace mcts;

template <typename State, typename Move>
void Simulation() {
	std::map<int8_t, size_t> stats;
	
	while (true) {
		MCTS<State, Move> ai1(1500, 3000);
		MCTS<State, Move> ai2(1500, 3000);

		State game;

		std::cout << game;

		while (!game.IsTerminal()) {
			if (game.GetPlayerID() == kOpponentID) {
                auto move = ai2.ParallelSearch();
				assert(game.IsLegalMove(move));
				std::cout << "AI2 turn! " << move << " rate:" << static_cast<int32_t>(ai2.GetCurrentNode()->GetWinRate() * 100) << "%\n";
				game.ApplyMove(move);
				ai1.SetOpponentMove(move);
			}
			else {
                auto move = ai1.ParallelSearch();
				assert(game.IsLegalMove(move));
				std::cout << "AI1 turn! " << move << " rate:" << static_cast<int32_t>(ai1.GetCurrentNode()->GetWinRate() * 100) << "%\n";
				game.ApplyMove(move);
				ai2.SetOpponentMove(move);
			}
			std::cout << game;
		}

		if (game.IsWinnerExsts()) {
			++stats[game.GetWinner()];
		}
		else {
            ++stats[State::kEmpty];
		}

        std::cout << State::kPlayer1 << " win:" << stats[State::kPlayer1] << "\n";
        std::cout << State::kPlayer2 << " win:" << stats[State::kPlayer2] << "\n";
        std::cout << "Tie" << " win:" << stats[State::kEmpty] << "\n";
	}
}

int main() {
    using namespace gomoku;
    Simulation<GomokuGameState, GomokuGameMove>();
    //using namespace tictactoe;
    //Simulation<TicTacToeGameState, TicTacToeGameMove>();
}
