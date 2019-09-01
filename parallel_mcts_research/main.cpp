#include <iostream>

#include "games/gomoku/gamestate.h"
#include "games/gomoku/gameserver.h"
#include "games/gomoku/gameclient.h"

int main() {
#if 1
	gomoku::GomokuGameServer server;
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
	const int32_t MAX_CLIENT = 1;
#else
	const int32_t MAX_CLIENT = 1;
#endif
	const auto scheme = "ws";
	const auto host = "127.0.0.1";
	const auto port = "9090";
	std::vector<std::shared_ptr<websocket::WebSocketClient>> clients;
	clients.reserve(MAX_CLIENT);
	for (auto i = 0; i < MAX_CLIENT; ++i) {
		auto ws = websocket::WebSocketClient::MakeSocket(
			scheme,
			host,
			port,
			ios,
			new gomoku::GomokuGameClientCallback());
		ws->Connect();
		clients.push_back(ws);
	}	
#endif	
	server.Run();

#else
	std::map<int8_t, size_t> stats;

	while (true) {
		mcts::MCTS<gomoku::GomokuGameState, gomoku::GomokuGameMove> ai1;
		mcts::MCTS<gomoku::GomokuGameState, gomoku::GomokuGameMove> ai2;
		gomoku::GomokuGameState game;

		ai1.Initial(3000, 3000);
		ai2.Initial(3000, 3000);

		std::cout << game;

		while (!game.IsTerminal()) {
			if (game.GetPlayerID() == 2) {
				auto move = ai2.ParallelSearch();
				assert(game.IsLegalMove(move));
				std::cout << "AI2 turn! winrate : " << ai2.GetCurrentNode()->GetWinRate() << "\n";
				game.ApplyMove(move);
				ai1.SetOpponentMove(move);
			}
			else {
				auto move = ai1.ParallelSearch();
				assert(game.IsLegalMove(move));
				std::cout << "AI1 turn! winrate : " << ai1.GetCurrentNode()->GetWinRate() << "\n";
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

		std::cout << gomoku::PLAYER1 << " win:" << stats[gomoku::PLAYER1] << "\n";
		std::cout << gomoku::PLAYER2 << " win:" << stats[gomoku::PLAYER2] << "\n";
		std::cout << "Tie" << " win:" << stats[gomoku::EMPTY] << "\n";
	}
#endif
}
