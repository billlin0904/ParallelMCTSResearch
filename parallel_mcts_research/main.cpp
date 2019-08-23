#include <array>
#include <map>
#include <iostream>
#include <cassert>
#include <fstream>
#include <sstream>
#include <unordered_set>

#include "rng.h"
#include "mcts.h"

#include "websocket/logger.h"
#include "websocket/websocket_client.h"
#include "websocket/websocket_server.h"

using namespace mcts;

class GomokuGameMove {
public:
    GomokuGameMove(int8_t row = 0, int8_t column = 0) noexcept
        : row(row)
        , column(column) {
    }

    int8_t row;
    int8_t column;

    std::string ToString() const {
        std::ostringstream ostr;
        ostr << int32_t(row) << "," << int32_t(column);
        return ostr.str();
    }

private:
    friend bool operator==(const GomokuGameMove& lhs, const GomokuGameMove& rhs) noexcept;
};

bool operator==(const GomokuGameMove& lhs, const GomokuGameMove& rhs) noexcept {
    return lhs.row == rhs.row && lhs.column == rhs.column;
}

static const int8_t PLAYER1 = 'O';
static const int8_t PLAYER2 = '@';
static const int8_t EMPTY = ' ';

static const int32_t MAX_WIDTH = 10;
static const int32_t MAX_HEIGHT = 10;

class GomokuGameState {
public:
    GomokuGameState() noexcept
        : winner_exists_(false)
        , is_terminal_(false)
        , player_id_(1)
        , remain_move_(MAX_WIDTH * MAX_HEIGHT)
        , board_(MAX_HEIGHT) {
        for (auto row = 0; row < MAX_WIDTH; ++row) {
            for (auto col = 0; col < MAX_HEIGHT; ++col) {
                board_[row].push_back(EMPTY);
            }
        }
    }

    bool IsTerminal() const noexcept {
        return is_terminal_;
    }

    bool IsWinnerExsts() const noexcept {
        return winner_exists_;
    }

    void ApplyMove(const GomokuGameMove& move) {
        if (player_id_ == 1) {
            board_[move.row][move.column] = PLAYER1;
        }
        else {
            board_[move.row][move.column] = PLAYER2;
        }
        --remain_move_;
        if (CheckWinner(move) != EMPTY) {
            winner_exists_ = true;
            is_terminal_ = true;
        }
        else {
            CheckTerminal();
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

    GomokuGameMove GetRandomMove() const {
        auto legal_moves = GetLegalMoves();
        assert(!legal_moves.empty());
        return legal_moves.at(RNG::Get()(0, int32_t(legal_moves.size() - 1)));
    }

    int8_t GetPlayerID() const noexcept {
        return player_id_;
    }

    void CheckTerminal() noexcept {
		if (winner_exists_) {
			return;
		}
        is_terminal_ = remain_move_ == 0;
    }

    bool IsLegalMove(const GomokuGameMove& move) const {
        if (move.row < MAX_WIDTH && move.column < MAX_HEIGHT) {
            return true;
        }
        return board_[move.row][move.column] == EMPTY;
    }

    std::vector<GomokuGameMove> GetLegalMoves() const noexcept {
        std::vector<GomokuGameMove> legal_moves;
        legal_moves.reserve(MAX_WIDTH * MAX_HEIGHT);
        for (auto row = 0; row < MAX_WIDTH; ++row) {
            for (auto col = 0; col < MAX_HEIGHT; ++col) {
                if (board_[row][col] == EMPTY) {
                    legal_moves.emplace_back(row, col);
                }
            }
        }
        return legal_moves;
    }

    std::string ToString() const {
        std::ostringstream ostr;
        for (auto row = 0; row < MAX_WIDTH; ++row) {
            for (auto col = 0; col < MAX_HEIGHT; ++col) {
                ostr << board_[row][col];
            }
        }
        return ostr.str();
    }

    int8_t GetWinner() const {
        for (auto row = 0; row < MAX_WIDTH; ++row) {
            for (auto col = 0; col < MAX_HEIGHT; ++col) {
                bool has_winner = false;
                if (board_[row][col] != EMPTY) {
                    if (Count(board_[row][col], row, col, 1, 0) >= 5) {
                        has_winner = true;
                    }
                    if (Count(board_[row][col], row, col, 0, 1) >= 5) {
                        has_winner = true;
                    }
                    if (Count(board_[row][col], row, col, 1, -1) >= 5) {
                        has_winner = true;
                    }
                    if (Count(board_[row][col], row, col, 1, 1) >= 5) {
                        has_winner = true;
                    }
                }
                if (has_winner) {
                    return board_[row][col];
                }
            }
        }
        return EMPTY;
    }

    int8_t CheckWinner(const GomokuGameMove& move) const noexcept {
        bool has_winner = false;
        if (board_[move.row][move.column] != EMPTY) {
            if (Count(board_[move.row][move.column], move.row, move.column, 1, 0) >= 5) {
                has_winner = true;
            }
            if (Count(board_[move.row][move.column], move.row, move.column, 0, 1) >= 5) {
                has_winner = true;
            }
            if (Count(board_[move.row][move.column], move.row, move.column, 1, -1) >= 5) {
                has_winner = true;
            }
            if (Count(board_[move.row][move.column], move.row, move.column, 1, 1) >= 5) {
                has_winner = true;
            }
            if (has_winner) {
                return board_[move.row][move.column];
            }
        }
        return EMPTY;
    }

private:
    int8_t Count(int8_t player, int32_t row, int32_t col, int32_t dirX, int32_t dirY) const noexcept {
        auto ct = 1;

        auto r = row + dirX;
        auto c = col + dirY;

        while (r >= 0 && r < MAX_WIDTH && c >= 0 && c < MAX_HEIGHT && board_[r][c] == player) {
            ct++;
            r += dirX;
            c += dirY;
        }

        r = row - dirX;
        c = col - dirY;

        while (r >= 0 && r < MAX_WIDTH && c >= 0 && c < MAX_HEIGHT && board_[r][c] == player) {
            ct++;
            r -= dirX;
            c -= dirY;
        }
        return ct;
    }

    friend std::ostream& operator<<(std::ostream& ostr, const GomokuGameState& state) {
        for (auto row = 0; row < MAX_WIDTH; ++row) {
            for (auto col = 0; col < MAX_HEIGHT; ++col) {
                ostr << state.board_[col][row] << "|";
            }
            ostr << "\n";
        }
        return ostr;
    }

    bool winner_exists_;
    bool is_terminal_;
    int8_t player_id_;
    int32_t remain_move_;
    GomokuGameMove last_move_;
    std::vector<std::vector<int8_t>> board_;
};

enum CommandID {
    ENTER_ROOM = 1000,
    START_ROUND,
    TURN,
};

static int32_t GetPacketID() noexcept {
    static std::atomic<int32_t> packet_id(10000);
    return ++packet_id;
}

static int32_t GetRoundID() noexcept {
    static std::atomic<int32_t> round_id(10000);
    return ++round_id;
}

static int32_t GetRoomID() noexcept {
    static std::atomic<int32_t> round_id(30000);
    return ++round_id;
}

static std::string GetJsonString(Document &document) {
    std::ostringstream ostr;
    OStreamWrapper wrapper{ ostr };
    PrettyWriter<OStreamWrapper> writer{ wrapper };
    document.Accept(writer);
    return ostr.str();
}

struct Encoder {
    static std::string EnterRoom(int32_t room_id, int32_t round_id, int32_t pid) {
        Document document;
        Value packet(kObjectType);
        packet.AddMember("cmd", CommandID::ENTER_ROOM, document.GetAllocator());
        packet.AddMember("room_id", room_id, document.GetAllocator());
        packet.AddMember("round_id", round_id, document.GetAllocator());
        packet.AddMember("player_id", pid, document.GetAllocator());
        packet.AddMember("pid", GetPacketID(), document.GetAllocator());
        document.SetObject();
        document.AddMember("packet", packet, document.GetAllocator());
        return GetJsonString(document);
    }

    static std::string StartRound(int32_t room_id, int32_t round_id, int32_t pid) {
        Document document;
        Value packet(kObjectType);
        packet.AddMember("cmd", CommandID::START_ROUND, document.GetAllocator());
        packet.AddMember("room_id", room_id, document.GetAllocator());
        packet.AddMember("round_id", round_id, document.GetAllocator());
        packet.AddMember("player_id", pid, document.GetAllocator());
        packet.AddMember("pid", GetPacketID(), document.GetAllocator());
        document.SetObject();
        document.AddMember("packet", packet, document.GetAllocator());
        return GetJsonString(document);
    }

    static std::string Turn(const GomokuGameMove& move,
                            const GomokuGameState& state,
                            const MCTS<GomokuGameState, GomokuGameMove> &mcts,
                            int32_t room_id,
                            int32_t round_id,
                            int32_t pid) {
        Document document;
        Value game_move(kObjectType);
        game_move.AddMember("row", move.row, document.GetAllocator());
        game_move.AddMember("column", move.column, document.GetAllocator());
        Value packet(kObjectType);
        packet.AddMember("cmd", CommandID::TURN, document.GetAllocator());
        packet.AddMember("room_id", room_id, document.GetAllocator());
        packet.AddMember("round_id", round_id, document.GetAllocator());
        packet.AddMember("player_id", state.GetPlayerID(), document.GetAllocator());
        packet.AddMember("pid", pid, document.GetAllocator());
        packet.AddMember("move", game_move, document.GetAllocator());
        document.SetObject();
        document.AddMember("packet", packet, document.GetAllocator());
        //mcts.WriteTo(document);
        return GetJsonString(document);
    }

	static std::string Turn(const GomokuGameMove& move,		
		int32_t player_id,
		int32_t room_id,
        int32_t round_id,
        int32_t pid) {
		Document document;
		Value game_move(kObjectType);
		game_move.AddMember("row", move.row, document.GetAllocator());
		game_move.AddMember("column", move.column, document.GetAllocator());
		Value packet(kObjectType);
		packet.AddMember("cmd", CommandID::TURN, document.GetAllocator());
		packet.AddMember("room_id", room_id, document.GetAllocator());
		packet.AddMember("round_id", round_id, document.GetAllocator());
		packet.AddMember("player_id", player_id, document.GetAllocator());
        packet.AddMember("pid", pid, document.GetAllocator());
		packet.AddMember("move", game_move, document.GetAllocator());
		document.SetObject();
		document.AddMember("packet", packet, document.GetAllocator());
		return GetJsonString(document);
	}
};

class GomokuRoom {
public:
    GomokuRoom(int32_t room_id, websocket::WebSocketServer *server)
        : room_id_(room_id)
        , round_id_(GetRoundID())
		, server_(server) {
		logger_ = websocket::Logger::Get().GetLogger("GomokuRoom");
    }

    void EnterRoom(int32_t session_id, bool is_watch) {
        auto pid = GetPacketID();
		if (players_.empty() && !is_watch) {
			players_.insert(session_id);
            server_->SentTo(session_id, Encoder::EnterRoom(room_id_, round_id_, pid));
		} else {
			watchers_.insert(session_id);
            BoardcastWatchers(Encoder::EnterRoom(room_id_, round_id_, pid));
			logger_->debug("Server send EnterRoom pid: {}", pid);
		}		
    }

    void NewRound(int32_t session_id) {
        ai_.reset(new MCTS<GomokuGameState, GomokuGameMove>());
        state_.reset(new GomokuGameState());
        round_id_ = GetRoundID();

        auto pid = GetPacketID();
        auto msg = Encoder::StartRound(room_id_, round_id_, pid);
		server_->SentTo(session_id, msg);
		BoardcastWatchers(msg);
		
		logger_->debug("==========> Server send NewRound pid: {} round id:{}", pid, round_id_);
    }

    void SetOpponentMove(const GomokuGameMove &move) {
        state_->ApplyMove(move);
        ai_->SetOpponentMove(move);

        auto pid = GetPacketID();
        auto msg = Encoder::Turn(move, 0, room_id_, round_id_, pid);
		BoardcastWatchers(msg);		
    }

    void ProcessTurn(int32_t session_id, const GomokuGameMove& move) {		
        if (!IsPlayerTurn(session_id)) {
            return;
        }
        if (IsTerminal()) {				
            NewRound(session_id);
            return;
        }
        SetOpponentMove(move);
        Turn(session_id);
        if (IsTerminal()) {
            NewRound(session_id);
        }
    }

	int32_t GetPlayerCount() const {
		return players_.size();
	}

	void LeavePlayer(int32_t session_id) {
		players_.erase(session_id);
		watchers_.erase(session_id);
	}

private:
    void Turn(int32_t session_id) {
#ifdef _DEBUG
        auto move = ai_->Search(30, 30);
#else
        auto move = ai_->Search(100, 300);
#endif
        assert(state_->IsLegalMove(move));
        state_->ApplyMove(move);
		std::cout << "Server move: " << move.ToString() << std::endl << *state_;

        auto pid = GetPacketID();
        auto msg = Encoder::Turn(move, *state_, *ai_, room_id_, round_id_, pid);
		logger_->debug("Server send Turn round: {} pid: {}", round_id_, pid);
        server_->SentTo(session_id, msg);
        BoardcastWatchers(msg);
    }

    bool IsTerminal() const {
        return state_->IsTerminal();
    }

    bool IsPlayerTurn(int32_t session_id) const {
        return players_.find(session_id) != players_.end();
    }

	void BoardcastWatchers(const std::string& message) {
		for (auto session_id : watchers_) {
			server_->SentTo(session_id, message);
		}
	}
    int32_t room_id_;
    int32_t round_id_;
	websocket::WebSocketServer* server_;
	std::unordered_set<int32_t> players_;
	std::unordered_set<int32_t> watchers_;
    std::unique_ptr<MCTS<GomokuGameState, GomokuGameMove>> ai_;
    std::unique_ptr<GomokuGameState> state_;
	std::shared_ptr<spdlog::logger> logger_;
};

class GomokuGameServer : public websocket::WebSocketServer {
public:
    GomokuGameServer()
        : websocket::WebSocketServer("Gomoku GameServer/1.0") {
		logger_ = websocket::Logger::Get().GetLogger("GomokuRoom");
    }

    void OnConnected(std::shared_ptr<websocket::Session> s) override {
        s->Receive();
    }

    void OnDisconnected(std::shared_ptr<websocket::Session> s) override {
        std::lock_guard<std::mutex> guard{ mutex_ };
        auto itr = session_room_.find(s->GetSessionID());
        if (itr != session_room_.end()) {
			Leave((*itr).second, s->GetSessionID());
        }
        RemoveSession(s);
    }

	void Leave(int32_t room_id, int32_t session_id) {
		auto itr = room_.find(room_id);
		if (itr == room_.end()) {
			return;
		}

		(*itr).second->LeavePlayer(session_id);
		if ((*itr).second->GetPlayerCount() == 0) {
			room_.erase(room_id);
			session_room_.erase(session_id);
		}
	}

    void OnSend(std::shared_ptr<websocket::Session>) override {
    }

    void OnReceive(std::shared_ptr<websocket::Session> s, const std::string& str) override {
        Document document;
        document.Parse(str);

        if (document.HasParseError()) {
            s->Receive();
            return;
        }

        const auto & packet = document["packet"];
        auto cmd = static_cast<CommandID>(packet["cmd"].GetInt());

        std::lock_guard<std::mutex> guard{ mutex_ };
        auto itr = session_room_.find(s->GetSessionID());

        switch (cmd) {
        case CommandID::ENTER_ROOM:
            RequestEnterRoom(packet, s, itr);
            break;
        case CommandID::TURN:			
            RequestTurn(s, packet, (*itr).second);
            break;
        case CommandID::START_ROUND:
        default:
            break;
        }
        s->Receive();
    }

    void OnError(std::shared_ptr<websocket::Session>, const websocket::Exception &e) override {
        std::cerr << e.what() << std::endl;
    }
private:
    void RequestEnterRoom(const Value &packet,
		const std::shared_ptr<websocket::Session> &s, 
		std::unordered_map<int32_t, int32_t>::iterator itr) {
		bool is_watch = false;

		if (packet.HasMember("is_watch")) {
			is_watch = packet["is_watch"].GetBool();
		}

		if (is_watch) {
			if (!session_room_.empty()) {
				auto itr = session_room_.begin();
				EnterRoom(s, (*itr).second, is_watch);
			}			
		} else {
			if (itr == session_room_.end()) {
				NewRoom(s, is_watch);
            } else {
				EnterRoom(s, (*itr).second, is_watch);
			}
		}
	}

    void EnterRoom(const std::shared_ptr<websocket::Session>& s, int32_t room_id, bool is_watch) {
        auto itr = room_.find(room_id);
        if (itr != room_.end()) {
			(*itr).second->EnterRoom(s->GetSessionID(), is_watch);
        }
    }

	void NewRoom(const std::shared_ptr<websocket::Session>& s, bool is_watch) {
        auto room_id = GetRoomID();
        session_room_[s->GetSessionID()] = room_id;
        std::unique_ptr<GomokuRoom> room(new GomokuRoom(room_id, this));
        room->NewRound(s->GetSessionID());
		room->EnterRoom(s->GetSessionID(), is_watch);
        room_.insert(std::make_pair(room_id, std::move(room)));
    }

    void RequestTurn(const std::shared_ptr<websocket::Session> &s, const Value& packet, int32_t room_id) {
        auto itr = room_.find(room_id);
        if (itr == room_.end()) {
			return;			
        }
        const GomokuGameMove move(packet["move"]["row"].GetInt(), packet["move"]["column"].GetInt());
		logger_->debug("Server receive client move: {} pid: {}", move.ToString(), packet["pid"].GetInt());
        (*itr).second->ProcessTurn(s->GetSessionID(), move);
    }

    std::mutex mutex_;	
    std::unordered_map<int32_t, int32_t> session_room_;
    std::unordered_map<int32_t, std::unique_ptr<GomokuRoom>> room_;
	std::shared_ptr<spdlog::logger> logger_;
};

class GomokuGameClientCallback : public websocket::WebSocketCallback {
public:
    GomokuGameClientCallback()
        : room_id_(0)
        , round_id_(0) {
		logger_ = websocket::Logger::Get().GetLogger("GomokuRoom");
    }

    void OnConnected(std::shared_ptr<websocket::WebSocketClient> s) override {
        state_.reset(new GomokuGameState());
        auto pid = GetPacketID();
        s->Send(Encoder::EnterRoom(room_id_, round_id_, pid));
        s->Receive();
    }

    void OnDisconnected(std::shared_ptr<websocket::WebSocketClient>) override {
    }

    void OnSend(std::shared_ptr<websocket::WebSocketClient>) override {
    }

    void OnReceive(std::shared_ptr<websocket::WebSocketClient> s, const std::string& str) override {
        Document document;
        document.Parse(str);

        if (document.HasParseError()) {
            s->Receive();
            return;
        }

        const auto& packet = document["packet"];
        auto cmd = static_cast<CommandID>(packet["cmd"].GetInt());
		logger_->debug("Client receive pid: {}", packet["pid"].GetInt());

        if (cmd == CommandID::ENTER_ROOM || cmd == CommandID::START_ROUND) {
            NewRoundOrEnterRoom(s, cmd);
        }
        else if (cmd == CommandID::TURN) {
            Turn(s, packet);
        }

		room_id_ = packet["room_id"].GetInt();
		round_id_ = packet["round_id"].GetInt();

        s->Receive();
		std::cout << "Client: " << room_id_ << " " << round_id_ << std::endl << *state_;
    }
    void OnError(std::shared_ptr<websocket::WebSocketClient>, const websocket::Exception &e) override {
		std::cerr << e.what() << std::endl;
    }
private:
    void NewRoundOrEnterRoom(const std::shared_ptr<websocket::WebSocketClient>& s, CommandID cmd) {
        if (cmd == CommandID::START_ROUND) {
			if (round_id_ > 0) {
				assert(state_->IsTerminal());
				state_.reset(new GomokuGameState());
				ai_.reset(new MCTS<GomokuGameState, GomokuGameMove>());
#ifdef _DEBUG        
				auto move = ai_->Search(10, 30);
#else
				auto move = ai_->Search(100, 30);
#endif
				state_->ApplyMove(move);
                auto pid = GetPacketID();
                s->Send(Encoder::Turn(move, *state_, *ai_, room_id_, round_id_, pid));
				logger_->debug("Client send Turn round_id: {} pid: {}", round_id_, pid);
			}            
		}
		else if (cmd == CommandID::ENTER_ROOM) {
			state_.reset(new GomokuGameState());
			ai_.reset(new MCTS<GomokuGameState, GomokuGameMove>());
#ifdef _DEBUG        
			auto move = ai_->Search(10, 30);
#else
			auto move = ai_->Search(100, 30);
#endif
			state_->ApplyMove(move);
            auto pid = GetPacketID();
            s->Send(Encoder::Turn(move, *state_, *ai_, room_id_, round_id_, pid));
			logger_->debug("Client send Turn round_id: {} pid: {}", round_id_, pid);
		}        
    }

    void Turn(const std::shared_ptr<websocket::WebSocketClient>& s, const Value &packet) {
        GomokuGameMove move(packet["move"]["row"].GetInt(), packet["move"]["column"].GetInt());
        state_->ApplyMove(move);
        ai_->SetOpponentMove(move);
#ifdef _DEBUG        
		auto search_move = ai_->Search(10, 30);
#else
		auto search_move = ai_->Search(100, 30);
#endif
        assert(state_->IsLegalMove(move));
        state_->ApplyMove(search_move);
        auto pid = GetPacketID();
        s->Send(Encoder::Turn(search_move, *state_, *ai_, room_id_, round_id_, pid));
		logger_->debug("Client send Turn round_id: {} pid: {}", round_id_, pid);
    }

    int32_t room_id_;
    int32_t round_id_;
    std::unique_ptr<MCTS<GomokuGameState, GomokuGameMove>> ai_;
    std::unique_ptr<GomokuGameState> state_;
	std::shared_ptr<spdlog::logger> logger_;
};

int main() {
	websocket::Logger::Get()
		.AddDebugOutputLogger()
		.AddFileLogger("gomoku.log");

    GomokuGameServer server;
    server.Bind("0.0.0.0", 9090);
    server.Listen();

#if 1
	boost::asio::io_service ios;
	std::vector<std::thread> thread_pool;

	for (auto i = 0; i < std::thread::hardware_concurrency(); ++i) {
		thread_pool.emplace_back([&]() {
			boost::asio::io_service::work work(ios);
			ios.run();
			});
	}	

    const auto scheme = "ws";
    const auto host = "127.0.0.1";
    const auto port = "9090";
    auto ws = websocket::WebSocketClient::MakeSocket(
		scheme,
        host,
        port,
        ios,
        new GomokuGameClientCallback());
    ws->Connect();
#endif

    server.Run();

#if 0
    std::map<int8_t, size_t> stats;

    while (true) {
        MCTS<GomokuGameState, GomokuGameMove> ai1;
        MCTS<GomokuGameState, GomokuGameMove> ai2;
        GomokuGameState game;

        std::cout << game;

        while (!game.IsTerminal()) {
            if (game.GetPlayerID() == 1) {
#if DEBUG
                auto move = ai2.Search(80, 400);
#else
                auto move = ai2.Search(80, 4000);
#endif
                assert(game.IsLegalMove(move));
                std::cout << "AI2 turn!\n";
                game.ApplyMove(move);
                ai1.SetOpponentMove(move);
            }
            else {
#if DEBUG
                auto move = ai1.Search(80, 400);
#else
                auto move = ai1.Search(80, 4000);
#endif
                assert(game.IsLegalMove(move));
                std::cout << "AI1 turn!\n";
                game.ApplyMove(move);
                ai2.SetOpponentMove(move);
#if DEBUG
                std::cout << ai1 << "\n";
#endif
            }
            std::cout << game;
        }

        std::cout << game;

        if (game.IsWinnerExsts()) {
            stats[game.GetWinner()]++;
        }
        else {
            stats[EMPTY]++;
        }

        std::cout << PLAYER1 << " win:" << stats[PLAYER1] << "\n";
        std::cout << PLAYER2 << " win:" << stats[PLAYER2] << "\n";
        std::cout << "Tie" << " win:" << stats[EMPTY] << "\n";
    }
#endif

}
