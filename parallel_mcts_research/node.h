#pragma once

#include <memory>
#include <vector>
#include <algorithm>
#include <cmath>

namespace mcts {

template <typename State, typename Move>
class Node;

template <typename State, typename Move>
using NodePtr = std::shared_ptr<Node<State, Move>>;

template <typename State, typename Move>
using WeakPtr = std::weak_ptr<Node<State, Move>>;

template <typename State, typename Move>
class Node : public std::enable_shared_from_this<Node<State, Move>> {
public:
    using self_type = Node<State, Move>;
    using ptr_type = NodePtr<State, Move>;
    using parent_ptr_type = WeakPtr<State, Move>;

    Node(const State& state = State(),
		const Move& move = Move(),
		NodePtr<State, Move> parent = nullptr)
        : player_id_(state.GetPlayerID())
		, score_(0)
		, visits_(0)
		, state_(state)
        , move_(move)
        , parent_(parent)
        , possible_moves_(state.GetLegalMoves()) {
    }

	void AddChild(const ptr_type &child) {
		children_.push_back(child);
	}

    ptr_type MakeChild(const Move &next_move) {
        State next_state(state_);
        next_state.ApplyMove(next_move);
        auto parent = this->shared_from_this();
        auto new_node = std::make_shared<self_type>(
                    next_state,
                    next_move,
                    parent);
        children_.push_back(new_node);
        RemoveMove(next_move);
        return new_node;
    }

    bool IsLeaf() const noexcept {
        return !children_.empty();
    }

    void Update(double result) noexcept {
        score_ += result;
        ++visits_;
    }

    void Update(double result, int64_t visits) noexcept {
        score_ += result;
        visits_ += visits;
    }

    bool HasPassibleMoves() const noexcept {
        return possible_moves_.empty();
    }

    double GetScore() const noexcept {
        return score_;
    }

	int64_t GetVisits() const noexcept {
        return visits_;
    }

    const State & GetState() const {
        return state_;
    }

    const std::vector<Move>& GetMoves() const noexcept {
        return possible_moves_;
    }

    const Move & GetLastMove() const noexcept {
        return move_;
    }

    int8_t GetPlayerID() const noexcept {
        return player_id_;
    }

    const std::vector<ptr_type>& GetChildren() const noexcept {
        return children_;
    }

    ptr_type GetParent() const {
        return parent_.lock();
    }

	size_t GetChildrenSize() const noexcept {
		return children_.size();
	}

	double GetUCB() const noexcept {
		return (GetScore() / GetVisits())
			+ UCBConstant() * std::sqrt(std::log(double(GetVisits())))
			/ double(GetVisits());
	}

private:
	static double UCBConstant() noexcept {
		static const double ucb = std::sqrt(2.0);
		return ucb;
	}

    void RemoveMove(const Move& move) {
		possible_moves_.erase(
			std::remove(possible_moves_.begin(), possible_moves_.end(), move),
			possible_moves_.end());
    }
    
    int8_t player_id_;
    double score_;
    int64_t visits_;
	State state_;
	Move move_;
    parent_ptr_type parent_;
    std::vector<ptr_type> children_;
    std::vector<Move> possible_moves_;
};

}
