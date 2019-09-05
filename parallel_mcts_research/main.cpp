// Copyright (c) 2019 ParallelMCTSResearch project.

#include <iostream>

#include "games/tictactoe/gamestate.h"
#include "games/tictactoe/gamemove.h"

#include "games/gomoku/gamestate.h"
#include "games/gomoku/gameserver.h"
#include "games/gomoku/gameclient.h"

using namespace websocket;

template <typename State, typename Move>
void Simulation() {
	std::map<int8_t, size_t> stats;

	while (true) {
		mcts::MCTS<State, Move> ai1;
		mcts::MCTS<State, Move> ai2;
		State game;

		ai1.Initial(300, 300);
		ai2.Initial(300, 300);

		std::cout << game;

		while (!game.IsTerminal()) {
			if (game.GetPlayerID() == 2) {
				auto move = ai2.ParallelSearch();
				assert(game.IsLegalMove(move));
				std::cout << "AI2 turn!" << "\n";
				game.ApplyMove(move);
				ai1.SetOpponentMove(move);
			}
			else {
				auto move = ai1.ParallelSearch();
				assert(game.IsLegalMove(move));
				std::cout << "AI1 turn!" << "\n";
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
#if 1
	using namespace gomoku;
	GomokuGameServer server;
	server.Bind("0.0.0.0", 9090);
	server.Listen();

	boost::asio::io_service ios;
	std::vector<std::thread> thread_pool;
	for (uint32_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
		thread_pool.emplace_back([&]() {
			boost::asio::io_service::work work(ios);
			ios.run();
			});
	}
#if 1
#ifdef _DEBUG
	const int32_t MAX_CLIENT = 1;
#else
	const int32_t MAX_CLIENT = 1;
#endif
	const auto scheme = "ws";
	const auto host = "127.0.0.1";
	const auto port = "9090";
	std::vector<std::shared_ptr<WebSocketClient>> clients;
	clients.reserve(MAX_CLIENT);
	for (auto i = 0; i < MAX_CLIENT; ++i) {
		auto ws = WebSocketClient::MakeSocket(
			scheme,
			host,
			port,
			ios,
			new GomokuGameClientCallback());
		ws->Connect();
		clients.push_back(ws);
	}	
#endif	
	server.Run();

#else
	using namespace gomoku;
	Simulation<GomokuGameState, GomokuGameMove>();
	//using namespace tictactoe;
	//Simulation<TicTacToeGameState, TicTacToeGameMove>();
#endif
}
