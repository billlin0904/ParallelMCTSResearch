#include <iostream>

#include "games/gomoku/gamestate.h"
#include "games/gomoku/gameserver.h"
#include "games/gomoku/gameclient.h"

using namespace gomoku;
using namespace websocket;

int main() {
#if 1
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
#if 0
#ifdef _DEBUG
	const int32_t MAX_CLIENT = 10;
#else
	const int32_t MAX_CLIENT = 10;
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
	std::map<int8_t, size_t> stats;

	while (true) {
		mcts::MCTS<GomokuGameState, GomokuGameMove> ai1;
		mcts::MCTS<GomokuGameState, GomokuGameMove> ai2;
		GomokuGameState game;

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
			stats[gomoku::EMPTY]++;
		}

		std::cout << PLAYER1 << " win:" << stats[PLAYER1] << "\n";
		std::cout << PLAYER2 << " win:" << stats[PLAYER2] << "\n";
		std::cout << "Tie" << " win:" << stats[EMPTY] << "\n";
	}
#endif
}
