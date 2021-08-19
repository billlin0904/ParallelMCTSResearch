// Copyright (c) 2019 ParallelMCTSResearch project.

#include <iostream>
#include <map>

#include "mcts.h"
#include "games/tictactoe/gamestate.h"
#include "games/tictactoe/gamemove.h"
#include "games/gomoku/gamestate.h"

using namespace mcts;

template <typename State, typename Move>
void Simulation() {
	std::map<int8_t, size_t> stats;
	
	while (true) {
		MCTS<State, Move> ai1;
		MCTS<State, Move> ai2;

		State game;

		ai1.Initial(1500, 1500);
		ai2.Initial(1500, 1500);

		std::cout << game;

		while (!game.IsTerminal()) {
			if (game.GetPlayerID() == 2) {
				auto move = ai2.ParallelSearch();
				assert(game.IsLegalMove(move));
				std::cout << "AI2 turn! " << move.ToString() << " rate:" << static_cast<int32_t>(ai2.GetCurrentNode()->GetWinRate() * 100) << "%\n";
				game.ApplyMove(move);
				ai1.SetOpponentMove(move);
			}
			else {
				auto move = ai1.ParallelSearch();
				assert(game.IsLegalMove(move));
				std::cout << "AI1 turn! " << move.ToString() << " rate:" << static_cast<int32_t>(ai1.GetCurrentNode()->GetWinRate() * 100) << "%\n";
				game.ApplyMove(move);
				ai2.SetOpponentMove(move);
			}
			std::cout << game;
		}

		std::cout << game;

		if (game.IsWinnerExsts()) {
			stats[game.GetWinner()]++;
		}
		else {
			stats[State::EMPTY]++;
		}

		std::cout << State::PLAYER1 << " win:" << stats[State::PLAYER1] << "\n";
		std::cout << State::PLAYER2 << " win:" << stats[State::PLAYER2] << "\n";
		std::cout << "Tie" << " win:" << stats[State::EMPTY] << "\n";
	}
}

int main() {
	using namespace gomoku;
	Simulation<GomokuGameState, GomokuGameMove>();
	//using namespace tictactoe;
	//Simulation<TicTacToeGameState, TicTacToeGameMove>();
}
