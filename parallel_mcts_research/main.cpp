#include <array>
#include <map>
#include <iostream>
#include <cassert>
#include <fstream>
#include <sstream>
#include <unordered_set>

#include "rng.h"
#include "mcts.h"
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

#if 0
static GomokuGameMove ParallelSearch(int32_t evaluate_count, int32_t rollout_limit) {
    std::vector<MCTS<GomokuGameState, GomokuGameMove>> mcst;

    mcst.resize(std::thread::hardware_concurrency());

    auto futus = ParallelFor(mcst.size(), [&mcst, evaluate_count, rollout_limit](int32_t i) {
        mcst[i].Search(evaluate_count, rollout_limit);
    });

    for (auto& f : futus) {
        f.get();
    }

    std::vector<MCTS<GomokuGameState, GomokuGameMove>::node_ptr_type> root;
    for (auto& m : mcst) {
        root.push_back(m.GetCurrentNode());
    }
    return GomokuGameMove();
}
#endif

enum CommandID {
    ENTER_ROOM = 1000,
    START_ROUND,
    TURN,
};

static std::string GetJsonString(Document &document) {
    std::ostringstream ostr;
    OStreamWrapper wrapper{ ostr };
    PrettyWriter<OStreamWrapper> writer{ wrapper };
    document.Accept(writer);
    return ostr.str();
}

struct Encoder {
    static std::string EnterRoom(const GomokuGameState& state, int32_t room_id, int32_t round_id) {
        Document document;
        Value packet(kObjectType);
        packet.AddMember("cmd", CommandID::ENTER_ROOM, document.GetAllocator());
        packet.AddMember("room_id", room_id, document.GetAllocator());
        packet.AddMember("round_id", round_id, document.GetAllocator());
        packet.AddMember("player_id", state.GetPlayerID(), document.GetAllocator());
        document.SetObject();
        document.AddMember("packet", packet, document.GetAllocator());
        return GetJsonString(document);
    }

    static std::string StartRound(const GomokuGameState& state, int32_t room_id, int32_t round_id) {
        Document document;
        Value packet(kObjectType);
        packet.AddMember("cmd", CommandID::START_ROUND, document.GetAllocator());
        packet.AddMember("room_id", room_id, document.GetAllocator());
        packet.AddMember("round_id", round_id, document.GetAllocator());
        packet.AddMember("player_id", state.GetPlayerID(), document.GetAllocator());
        document.SetObject();
        document.AddMember("packet", packet, document.GetAllocator());
        return GetJsonString(document);
    }

    static std::string Turn(const GomokuGameMove& move,
                            const GomokuGameState& state,
                            const MCTS<GomokuGameState, GomokuGameMove> &mcts,
                            int32_t room_id,
                            int32_t round_id) {
        Document document;
        Value game_move(kObjectType);
        game_move.AddMember("row", move.row, document.GetAllocator());
        game_move.AddMember("column", move.column, document.GetAllocator());
        Value packet(kObjectType);
        packet.AddMember("cmd", CommandID::TURN, document.GetAllocator());
        packet.AddMember("room_id", room_id, document.GetAllocator());
        packet.AddMember("round_id", round_id, document.GetAllocator());
        packet.AddMember("player_id", state.GetPlayerID(), document.GetAllocator());
        packet.AddMember("move", game_move, document.GetAllocator());
        document.SetObject();
        document.AddMember("packet", packet, document.GetAllocator());
        mcts.WriteTo(document);
        return GetJsonString(document);
    }
};

static int32_t GetRoundID() noexcept {
    static std::atomic<int32_t> round_id(10000);
    return ++round_id;
}

static int32_t GetRoomID() noexcept {
    static std::atomic<int32_t> round_id(30000);
    return ++round_id;
}

class GomokuRoom {
public:
    GomokuRoom(int32_t room_id, websocket::WebSocketServer *server)
        : room_id_(room_id)
        , round_id_(GetRoundID())
		, server_(server) {
    }

    void EnterRoom(int32_t session_id) {
		if (!players_.empty()) {
			players_.insert(session_id);
			server_->SentTo(session_id, Encoder::EnterRoom(*state_, room_id_, round_id_));
		} else {
			watchers_.insert(session_id);
			SendWatchers(Encoder::EnterRoom(*state_, room_id_, round_id_));
		}		
    }

    std::string NewRound() {
        ai_.reset(new MCTS<GomokuGameState, GomokuGameMove>());
        state_.reset(new GomokuGameState());
        round_id_ = GetRoundID();
        return Encoder::StartRound(*state_, room_id_, round_id_);
    }

    void SetOpponentMove(const GomokuGameMove &move) {
        state_->ApplyMove(move);
        ai_->SetOpponentMove(move);
    }

    std::string Turn() {
        std::cout << "Server:" << std::endl << *state_ << std::endl;
        auto move = ai_->Search(100, 3000);
        state_->ApplyMove(move);
        std::cout << "Server move: " << move.ToString() << std::endl << *state_ << std::endl;
        return Encoder::Turn(move, *state_, *ai_, room_id_, round_id_);
    }

    bool IsTerminal() const {
        return state_->IsTerminal();
    }

	void LeaveWatcher(int32_t session_id) {
		watchers_.erase(session_id);
	}

private:
	void SendWatchers(const std::string& message) {
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
};

class GomokuGameServer : public websocket::WebSocketServer {
public:
    GomokuGameServer()
        : websocket::WebSocketServer("Gomoku GameServer/1.0") {
    }

    void OnConnected(std::shared_ptr<websocket::Session> s) override {
        s->Receive();
        std::cout << "Seesion id: " << s->GetSessionID() << " connected!" << std::endl;
    }

    void OnDisconnected(std::shared_ptr<websocket::Session> s) override {
        std::lock_guard<std::mutex> guard{ mutex_ };
        auto itr = session_room_.find(s->GetSessionID());
        if (itr != session_room_.end()) {
            room_.erase((*itr).second);
            session_room_.erase(itr);
        }
        RemoveSession(s);
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
            if (itr == session_room_.end()) {
                NewRoom(s);
            } else {
                EnterRoom(s, (*itr).second);
            }
            break;
        case CommandID::TURN:			
            Turn(s, packet, (*itr).second);
            break;
        }
        s->Receive();
    }

    void OnError(std::shared_ptr<websocket::Session>, const websocket::Exception &e) override {
        std::cerr << e.what() << std::endl;
    }
private:
    void EnterRoom(const std::shared_ptr<websocket::Session>& s, int32_t room_id) {
        auto itr = room_.find(room_id);
        if (itr == room_.end()) {
			(*itr).second->EnterRoom(s->GetSessionID());
        }
    }

    void NewRoom(const std::shared_ptr<websocket::Session>& s) {
        auto room_id = GetRoomID();
        session_room_[s->GetSessionID()] = room_id;
        std::unique_ptr<GomokuRoom> room(new GomokuRoom(room_id, this));
        room->NewRound();
		room->EnterRoom(s->GetSessionID());
        room_.insert(std::make_pair(room_id, std::move(room)));
    }

    void Turn(const std::shared_ptr<websocket::Session> &s, const Value& packet, int32_t room_id) {
        auto itr = room_.find(room_id);
        if (itr != room_.end()) {
			if ((*itr).second->IsTerminal()) {
				s->Send((*itr).second->NewRound());
				return;
			}
            const GomokuGameMove move(packet["move"]["row"].GetInt(), packet["move"]["column"].GetInt());
            std::cout << "Client move: " << move.ToString() << std::endl;
            (*itr).second->SetOpponentMove(move);
            s->Send((*itr).second->Turn());
			if ((*itr).second->IsTerminal()) {
				s->Send((*itr).second->NewRound());
			}
        }
    }

    std::mutex mutex_;
    std::unordered_map<int32_t, int32_t> session_room_;
    std::unordered_map<int32_t, std::unique_ptr<GomokuRoom>> room_;
};

class GomokuGameClientCallback : public websocket::WebSocketCallback {
public:
    GomokuGameClientCallback()
        : room_id_(0)
        , round_id_(0) {
    }

    void OnConnected(std::shared_ptr<websocket::WebSocketClient> s) override {
        state_.reset(new GomokuGameState());
        s->Send(Encoder::EnterRoom(*state_, room_id_, round_id_));
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
        room_id_ = packet["room_id"].GetInt();
        round_id_ = packet["round_id"].GetInt();

        if (cmd == CommandID::ENTER_ROOM || cmd == CommandID::START_ROUND) {
            NewRoundOrEnterRoom(s, cmd);
        }
        else if (cmd == CommandID::TURN) {
            Turn(s, packet);
        }

        std::cout << "Client: " << room_id_ << " - " << round_id_ << std::endl << *state_ << std::endl;
        s->Receive();
    }
    void OnError(std::shared_ptr<websocket::WebSocketClient>, websocket::OperatorError, boost::system::error_code) override {

    }
private:
    void NewRoundOrEnterRoom(std::shared_ptr<websocket::WebSocketClient> s, CommandID cmd) {
        if (cmd == CommandID::START_ROUND) {
            assert(state_->IsTerminal());
            state_.reset(new GomokuGameState());
        }
        ai_.reset(new MCTS<GomokuGameState, GomokuGameMove>());
        auto move = ai_->Search(80, 400);
        state_->ApplyMove(move);
        s->Send(Encoder::Turn(move, *state_, *ai_, room_id_, round_id_));
    }

    void Turn(std::shared_ptr<websocket::WebSocketClient> s, const Value &packet) {
        GomokuGameMove move(packet["move"]["row"].GetInt(), packet["move"]["column"].GetInt());
        state_->ApplyMove(move);
        ai_->SetOpponentMove(move);

        auto search_move = ai_->Search(80, 400);
        state_->ApplyMove(search_move);
        s->Send(Encoder::Turn(search_move, *state_, *ai_, room_id_, round_id_));
    }

    int32_t room_id_;
    int32_t round_id_;
    std::unique_ptr<MCTS<GomokuGameState, GomokuGameMove>> ai_;
    std::unique_ptr<GomokuGameState> state_;
};

int main() {
    GomokuGameServer server;
    server.Bind("0.0.0.0", 9090);
    server.Listen();

    /*
    boost::asio::io_service ios;
    std::thread client_thread([&]() {
        boost::asio::io_service::work work(ios);
        ios.run();
    });


    const auto scheme = "ws";
    const auto host = "127.0.0.1";
    const auto port = "9090";
    auto ws = websocket::WebSocket::MakeSocket(scheme,
                                               host,
                                               port,
                                               ios,
                                               new GomokuGameClientCallback());
    ws->Connect();
    */

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
