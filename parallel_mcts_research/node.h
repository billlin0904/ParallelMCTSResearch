// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <memory>
#include <vector>
#include <algorithm>

#include "tweakme.h"

namespace mcts {

enum {
	kPlayerID = 1,
    kOpponentID = 2,
};

template <typename State, typename Move, typename UCB1Policy>
class Node;

template <typename State, typename Move, typename UCB1Policy>
using NodePtr = std::shared_ptr<Node<State, Move, UCB1Policy>>;

template <typename State, typename Move, typename UCB1Policy>
using WeakPtr = std::weak_ptr<Node<State, Move, UCB1Policy>>;

template <typename State, typename Move, typename UCB1Policy>
class Node 
	: public std::enable_shared_from_this<Node<State, Move, UCB1Policy>> {
public:
    using self_type = Node<State, Move, UCB1Policy>;
    using ptr_type = NodePtr<State, Move, UCB1Policy>;
    using parent_ptr_type = WeakPtr<State, Move, UCB1Policy>;

    explicit Node(State state = State(),
                  Move move = Move(),
                  ptr_type parent = nullptr)
        : player_id_(state.GetPlayerID())        
        , move_(move)
        , parent_(parent)
        , possible_moves_(state.GetLegalMoves())
		, board_states_(state) {
    }

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    ptr_type MakeChild(const Move &next_move) {
        State next_state(board_states_);
        next_state.ApplyMove(next_move);
        auto parent = this->shared_from_this();
        auto new_node = std::make_shared<self_type>(
                    next_state,
                    next_move,
                    parent);
        children_.push_back(new_node);
        possible_moves_.erase(next_move);
        return new_node;
    }

    [[nodiscard]] bool IsLeaf() const noexcept {
        return !children_.empty();
    }

    [[nodiscard]] bool HasPassibleMoves() const noexcept {
		return !possible_moves_.empty();
	}

	void Update(double score, int32_t visits) noexcept {
		ucb1_policy_.Update(score, visits);
	}    

    [[nodiscard]] double GetScore() const noexcept {
		return ucb1_policy_.GetScore();
    }

    [[nodiscard]] int64_t GetVisits() const noexcept {
        return ucb1_policy_.GetVisits();
    }

    const State & GetState() const {
        return board_states_;
    }

	const HashSet<Move>& GetMoves() const noexcept {
		return possible_moves_;
	}

    const Move & GetLastMove() const noexcept {
        return move_;
    }

    [[nodiscard]] int8_t GetPlayerID() const noexcept {
        return player_id_;
    }

    const std::vector<ptr_type>& GetChildren() const noexcept {
        return children_;
    }

    ptr_type GetParent() const {
        return parent_.lock();
    }

    [[nodiscard]] size_t GetChildrenSize() const noexcept {
        return children_.size();
    }

    [[nodiscard]] double GetUCB() const noexcept {
        return ucb1_policy_(GetParent()->GetVisits());
    }

    [[nodiscard]] double GetWinRate() const {
		return GetScore() / static_cast<double>(GetVisits());
	}

private:
    int8_t player_id_ : 2;       
    Move move_;
    parent_ptr_type parent_;
	UCB1Policy ucb1_policy_;
    std::vector<ptr_type> children_;
    HashSet<Move> possible_moves_;
	State board_states_;
};

}
