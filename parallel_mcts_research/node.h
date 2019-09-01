#pragma once

#include <memory>
#include <vector>
#include <algorithm>

#include "tweakme.h"

namespace mcts {

#if USE_STD_UNORDERED_SET
template <typename T>
using HashSet = phmap::unordered_set<T>;
#else
template <typename T>
using HashSet = phmap::flat_hash_set<T>;
#endif

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

    Node(const State& state = State(),
         const Move& move = Move(),
         NodePtr<State, Move, UCB1Policy> parent = nullptr)
        : player_id_(state.GetPlayerID())        
        , move_(move)
        , parent_(parent)
        , possible_moves_(state.GetLegalMoves())
		, state_(state) {
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

	inline bool IsLeaf() const noexcept {
        return !children_.empty();
    }

	inline bool HasPassibleMoves() const noexcept {
		return possible_moves_.empty();
	}

    void Update(double score) noexcept {
		ucb1_policy_.Update(score);
    }

	void Update(double score, int64_t visits) noexcept {
		ucb1_policy_.Update(score, visits);
	}    

    double GetScore() const noexcept {
		return ucb1_policy_.GetScore();
    }

    int64_t GetVisits() const noexcept {
        return ucb1_policy_.GetVisits();
    }

    const State & GetState() const {
        return state_;
    }

	const HashSet<Move>& GetMoves() const noexcept {
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
        return ucb1_policy_(GetParent()->GetVisits());
    }

	inline double GetWinRate() const {
		return GetScore() / double(GetVisits());
	}

private:
    void RemoveMove(const Move& move) {
		possible_moves_.erase(move);
    }
    
    int8_t player_id_;       
    Move move_;
    parent_ptr_type parent_;
	UCB1Policy ucb1_policy_;
    std::vector<ptr_type> children_;
	HashSet<Move> possible_moves_;
	State state_;
};

}
