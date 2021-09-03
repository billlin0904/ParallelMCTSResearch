 // Copyright (c) 2019 ParallelMCTSResearch project.

#include <iostream>
#include <map>

#include "mcts.h"
#include "games/gomoku/gamestate.h"

using namespace mcts;

template <typename State, typename Move>
std::map<int8_t, size_t> Simulation(int32_t count, bool is_show_game) {
	ThreadPool select_tp;
	ThreadPool rollout_tp;

	std::map<int8_t, size_t> stats;
	
	for (auto i = 0; i < count; ++i) {
#ifdef _DEBUG
		MCTS<State, Move> ai1(1500, 1500);
		MCTS<State, Move> ai2(1500, 1500);
#else
		MCTS<State, Move> ai1(85000, 15000);
		MCTS<State, Move> ai2(85000, 15000);
#endif
		State game;

		if (is_show_game) {
			std::cout << "Game start!" << std::endl;
		}

		for (auto play_count = 0; !game.IsTerminal(); ++play_count) {
			if (game.GetPlayerID() == kOpponentID) {
                auto move = ai2.ParallelSearch(select_tp, rollout_tp);
				assert(game.IsLegalMove(move));
				if (is_show_game) {
					std::cout << "AI2 turn! " << State::kPlayer2 << " " << move
					<< " rate:" << static_cast<int32_t>(ai2.GetCurrentNode()->GetWinRate() * 100) << "%\n";
				}
				game.ApplyMove(move);
				ai1.SetOpponentMove(move);
			}
			else {
				Move move(0, 0);
				if (play_count == 0) {
					move = Move(4, 4);
				} else if (play_count == 2) {
					move = Move(1, 4);
				} else {
					move = ai1.ParallelSearch(select_tp, rollout_tp);
				}
				assert(game.IsLegalMove(move));
				game.ApplyMove(move);
				ai2.SetOpponentMove(move);
				if (is_show_game) {
					if (play_count <= 2) {
						if (play_count == 2) {
							ai1.GetCurrentNode()->SetState(game);
						}
						std::cout << "AI1 turn! " << move << "\n";
					}					
					else {
						std::cout << "AI1 turn! " << State::kPlayer1 << " " << move
						<< " rate:" << static_cast<int32_t>(ai1.GetCurrentNode()->GetWinRate() * 100) << "%\n";
					}
				}
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

		if (is_show_game) {
			std::cout << State::kPlayer1 << " win:" << stats[State::kPlayer1] << "\n";
			std::cout << State::kPlayer2 << " win:" << stats[State::kPlayer2] << "\n";
			std::cout << "Tie" << " win:" << stats[State::kEmpty] << "\n";
		}		
	}
	return stats;
}

template <typename State, typename Move>
void Gomoku(int32_t count = 1000, bool is_show_game = false) {
	std::map<int8_t, size_t> stats = Simulation<State, Move>(count, is_show_game);
	std::cout << State::kPlayer1 << " win:" << stats[State::kPlayer1] << "\n";
	std::cout << State::kPlayer2 << " win:" << stats[State::kPlayer2] << "\n";
	std::cout << "Tie" << " win:" << stats[State::kEmpty] << "\n";
}

int main() {
    using namespace gomoku;
	Gomoku<GomokuGameState, GomokuGameMove>(1000, true);
	std::cin.get();
}
