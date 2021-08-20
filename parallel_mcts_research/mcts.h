// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <cassert>


#include "tweakme.h"
#include "rng.h"
#include "node.h"
#include "ucbpolicy.h"

namespace mcts {

using namespace std::chrono;

template <typename State, typename Move, typename UCB1Policy = DefaultUCB1Policy>
class MCTS {
public:
    static constexpr int32_t kMaxEvaluateCount = 64;
    static constexpr int32_t kMaxRolloutCount = 128;

    using node_type = Node<State, Move, UCB1Policy>;
    using node_ptr_type = typename Node<State, Move, UCB1Policy>::ptr_type;
    using children_vector_type = std::vector<node_ptr_type>;

    MCTS(int32_t evaluate_count = kMaxEvaluateCount, int32_t rollout_limit = kMaxRolloutCount);

    explicit MCTS(const State& state, int32_t evaluate_count = kMaxEvaluateCount, int32_t rollout_limit = kMaxRolloutCount);

    MCTS(const MCTS&) = default;
    MCTS& operator=(const MCTS &) = default;

    void SetSearchLimit(int32_t evaluate_count, int32_t rollout_limit);

    Move Search(milliseconds search_time = milliseconds(5000));

    Move ParallelSearch(milliseconds search_time = milliseconds(5000));

    void SetOpponentMove(const Move& opponent_move);

	const node_ptr_type& GetCurrentNode() const;

private:
    node_ptr_type GetBestChild(const node_ptr_type& parent) const;

    node_ptr_type GetBestUCBChild(const node_ptr_type& parent) const;

    node_ptr_type Select() const;

    node_ptr_type Expand(node_ptr_type& node);

    double Rollout(const node_ptr_type& leaf);

    void BackPropagation(const node_ptr_type& leaf, double score);

	std::atomic<bool> cancelled_;
    int32_t evaluate_count_;
    int32_t rollout_limit_;
    node_ptr_type root_;
    node_ptr_type current_node_;
    std::mutex root_mutex_;
};

template <typename State, typename Move, typename UCB1Policy>
MCTS<State, Move, UCB1Policy>::MCTS(int32_t evaluate_count, int32_t rollout_limit)
    : cancelled_(false)
	, evaluate_count_(evaluate_count)
    , rollout_limit_(rollout_limit)
    , root_(std::make_shared<Node<State, Move, UCB1Policy>>())
    , current_node_(root_) {
}

template <typename State, typename Move, typename UCB1Policy>
MCTS<State, Move, UCB1Policy>::MCTS(const State &state, int32_t evaluate_count, int32_t rollout_limit)
    : cancelled_(false)
	, evaluate_count_(evaluate_count)
    , rollout_limit_(rollout_limit)
    , root_(std::make_shared<Node<State, Move, UCB1Policy>>(state))
    , current_node_(root_) {
}

template <typename State, typename Move, typename UCB1Policy>
void MCTS<State, Move, UCB1Policy>::SetSearchLimit(int32_t evaluate_count, int32_t rollout_limit) {
    evaluate_count_ = evaluate_count;
    rollout_limit_ = rollout_limit;
}

template <typename State, typename Move, typename UCB1Policy>
const typename MCTS<State, Move, UCB1Policy>::node_ptr_type& MCTS<State, Move, UCB1Policy>::GetCurrentNode() const {
    return current_node_;
}

template <typename State, typename Move, typename UCB1Policy>
typename MCTS<State, Move, UCB1Policy>::node_ptr_type MCTS<State, Move, UCB1Policy>::GetBestUCBChild(const node_ptr_type
	& parent) const {
    const auto& candidate_node = parent->GetChildren();
    auto itr = std::max_element(candidate_node.cbegin(), candidate_node.cend(), [](
                                const auto& first,
                                const auto& last) noexcept {
        return first->GetUCB() > last->GetUCB();
    });
    return *itr;
}

template <typename State, typename Move, typename UCB1Policy>
typename MCTS<State, Move, UCB1Policy>::node_ptr_type MCTS<State, Move, UCB1Policy>::GetBestChild(const node_ptr_type& parent) const {
    const auto& candidate_node = parent->GetChildren();
    auto itr = std::max_element(candidate_node.cbegin(), candidate_node.cend(),
                                [](
								const auto& large,
                                const auto& node) noexcept {
		return node->GetWinRate() > large->GetWinRate();
    });
    return *itr;
}

template <typename State, typename Move, typename UCB1Policy>
Move MCTS<State, Move, UCB1Policy>::Search(milliseconds search_time) {
	cancelled_ = false;
    auto start_tp = steady_clock::now();

    for (auto i = 0; i < evaluate_count_
        && duration_cast<milliseconds>(steady_clock::now() - start_tp) < search_time; ++i) {
        auto selected_parent = Select();
        auto selected_leaf = Expand(selected_parent);
        auto score = Rollout(selected_leaf);
        BackPropagation(selected_leaf, score);
    }

    current_node_ = GetBestChild(current_node_);
    return current_node_->GetLastMove();
}

template <typename State, typename Move, typename UCB1Policy>
Move MCTS<State, Move, UCB1Policy>::ParallelSearch(milliseconds search_time) {
    cancelled_ = false;
    auto start_tp = steady_clock::now();
    mcts::ParallelFor(evaluate_count_, [this, start_tp, search_time](int32_t) {
        cancelled_ = duration_cast<milliseconds>(steady_clock::now() - start_tp) > search_time;
        if (cancelled_) {
            return;
        }
        node_ptr_type selected_leaf;
        node_ptr_type selected_parent;
        {
            std::lock_guard guard{ root_mutex_ };
            selected_parent = Select();
            selected_leaf = Expand(selected_parent);
        }
        auto score = Rollout(selected_leaf);
        std::lock_guard guard{ root_mutex_ };
        BackPropagation(selected_leaf, score);
    });
    current_node_ = GetBestChild(current_node_);
    return current_node_->GetLastMove();
}

template <typename State, typename Move, typename UCB1Policy>
void MCTS<State, Move, UCB1Policy>::SetOpponentMove(const Move& opponent_move) {
    const auto& available_moves = current_node_->GetMoves();
    auto itr = std::find(available_moves.cbegin(),
                         available_moves.cend(),
                         opponent_move);
    if (itr != available_moves.end()) {
        current_node_ = current_node_->MakeChild(opponent_move);
    }
    else {
        const auto & children = current_node_->GetChildren();
        auto child = std::find_if(children.begin(), 
								children.end(),[&opponent_move](
                                const auto& node) {
            return node->GetLastMove() == opponent_move;
        });
        assert(child != children.end());
        if (child != children.end()) {
            current_node_ = *child;
        }
    }
}

template <typename State, typename Move, typename UCB1Policy>
typename MCTS<State, Move, UCB1Policy>::node_ptr_type MCTS<State, Move, UCB1Policy>::Select() const {
    auto selected_node = current_node_;
    while (!selected_node->HasPassibleMoves() && selected_node->IsLeaf()) {
        selected_node = GetBestUCBChild(selected_node);
    }
    return selected_node;
}

template <typename State, typename Move, typename UCB1Policy>
typename MCTS<State, Move, UCB1Policy>::node_ptr_type MCTS<State, Move, UCB1Policy>::Expand(node_ptr_type& parent) {
    if (parent->HasPassibleMoves()) {
        const auto& available_moves = parent->GetMoves();
		auto itr = std::next(std::begin(available_moves), RNG::Get()(0, static_cast<int32_t>(available_moves.size() - 1)));
		return parent->MakeChild(*itr);
    }
    if (parent->IsLeaf()) {
	    const auto& children = parent->GetChildren();
	    auto idx = RNG::Get()(static_cast<size_t>(0), children.size() - 1);
	    return children[idx];
    }
    return parent;
}

template <typename State, typename Move, typename UCB1Policy>
double MCTS<State, Move, UCB1Policy>::Rollout(const node_ptr_type& leaf) {
	auto total_score = 0.0;
    for (auto i = 0; i < rollout_limit_; ++i) {
        auto state = leaf->GetState();
        while (!state.IsTerminal()) {
            state.ApplyMove(state.GetRandomMove());
        }
        double result = state.Evaluate();
        result = 0.5 * (result + 1) * (current_node_->GetPlayerID() == kPlayerID)
                + 0.5 * (1 - result) * (current_node_->GetPlayerID() == kOpponentID);
        total_score += result;
    }
    return total_score;
}

template <typename State, typename Move, typename UCB1Policy>
void MCTS<State, Move, UCB1Policy>::BackPropagation(const node_ptr_type& leaf, double score) {
    leaf->Update(score, rollout_limit_);
    auto parent = leaf->GetParent();
    if (!parent) {
        return;
    }

    std::vector<node_ptr_type> parents;
    parents.push_back(parent);

    while (true) {
        parent = parent->GetParent();
        if (parent != nullptr) {
            parents.push_back(parent);
        } else {
            break;
        }
    }

    for (auto& parent : parents) {
        parent->Update(score, rollout_limit_);
    }
}

}

