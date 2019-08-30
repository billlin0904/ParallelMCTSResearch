#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <ostream>

#include "rng.h"
#include "node.h"
#include "ucbpolicy.h"

#define EANBE_PPL 1

#if EANBE_PPL && defined(_WIN32)
#include <ppl.h>
#else
#include "threadpool.h"
#endif

namespace mcts {

template <typename State, typename Move, typename UCB1Policy = DefaultUCB1Policy>
class MCTS {
public:
    using node_type = Node<State, Move, UCB1Policy>;
    using node_ptr_type = typename Node<State, Move, UCB1Policy>::ptr_type;
    using children_vector_type = std::vector<node_ptr_type>;

    MCTS(int32_t evaluate_count = 64, int32_t rollout_limit = 128);

    explicit MCTS(const State& state, int32_t evaluate_count = 64, int32_t rollout_limit = 128);

    MCTS(const MCTS&) = default;
    MCTS& operator=(const MCTS &) = default;

    void Initial(int32_t evaluate_count, int32_t rollout_limit);

    Move Search();

    Move ParallelSearch();

    void SetOpponentMove(const Move& opponent_move);

    node_ptr_type GetCurrentNode() const;

    node_ptr_type GetRoot() const;

private:
    node_ptr_type GetBestChild(const node_ptr_type& parent) const;

    node_ptr_type GetBestUCBChild(const node_ptr_type& parent) const;

    node_ptr_type Select() const;

    node_ptr_type Expand(node_ptr_type& node);

    double Rollout(const node_ptr_type& leaf);

    void BackPropagation(const node_ptr_type& leaf, double score);

    int32_t evaluate_count_;
    int32_t rollout_limit_;
    node_ptr_type root_;
    node_ptr_type current_node_;
    std::mutex root_mutex_;
};

template <typename State, typename Move, typename UCB1Policy>
MCTS<State, Move, UCB1Policy>::MCTS(int32_t evaluate_count, int32_t rollout_limit)
    : evaluate_count_(evaluate_count)
    , rollout_limit_(rollout_limit)
    , root_(std::make_shared<Node<State, Move, UCB1Policy>>())
    , current_node_(root_) {
}

template <typename State, typename Move, typename UCB1Policy>
MCTS<State, Move, UCB1Policy>::MCTS(const State &state, int32_t evaluate_count, int32_t rollout_limit)
    : evaluate_count_(evaluate_count)
    , rollout_limit_(rollout_limit)
    , root_(std::make_shared<Node<State, Move, UCB1Policy>>(state))
    , current_node_(root_) {
}

template <typename State, typename Move, typename UCB1Policy>
void MCTS<State, Move, UCB1Policy>::Initial(int32_t evaluate_count, int32_t rollout_limit) {
    evaluate_count_ = evaluate_count;
    rollout_limit_ = rollout_limit;
}

template <typename State, typename Move, typename UCB1Policy>
typename MCTS<State, Move, UCB1Policy>::node_ptr_type MCTS<State, Move, UCB1Policy>::GetRoot() const {
    return root_;
}

template <typename State, typename Move, typename UCB1Policy>
typename MCTS<State, Move, UCB1Policy>::node_ptr_type MCTS<State, Move, UCB1Policy>::GetBestUCBChild(const typename MCTS<State, Move, UCB1Policy>::node_ptr_type& parent) const {
    const auto& candidate_node = parent->GetChildren();
    auto itr = std::max_element(candidate_node.begin(), candidate_node.end(), [this](
                                const typename MCTS<State, Move, UCB1Policy>::node_ptr_type &first,
                                const typename MCTS<State, Move, UCB1Policy>::node_ptr_type& last) {
        return first->GetUCB() > last->GetUCB();
    });
    return *itr;
}

template <typename State, typename Move, typename UCB1Policy>
typename MCTS<State, Move, UCB1Policy>::node_ptr_type MCTS<State, Move, UCB1Policy>::GetBestChild(const typename MCTS<State, Move, UCB1Policy>::node_ptr_type& parent) const {
    const auto& candidate_node = parent->GetChildren();
    auto itr = std::max_element(candidate_node.begin(), candidate_node.end(),
                                [](const typename MCTS<State, Move, UCB1Policy>::node_ptr_type& large,
                                const typename MCTS<State, Move, UCB1Policy>::node_ptr_type& node) {
        return ((node->GetScore() / double(node->GetVisits()))
                > (large->GetScore() / double(large->GetVisits())));
    });
    return *itr;
}

template <typename State, typename Move, typename UCB1Policy>
typename MCTS<State, Move, UCB1Policy>::node_ptr_type MCTS<State, Move, UCB1Policy>::GetCurrentNode() const {
    return current_node_;
}

template <typename State, typename Move, typename UCB1Policy>
Move MCTS<State, Move, UCB1Policy>::Search() {
    for (auto i = 0; i < evaluate_count_; ++i) {
        auto selected_parent = Select();
        auto selected_leaf = Expand(selected_parent);
        auto score = Rollout(selected_leaf);
        BackPropagation(selected_leaf, score);
    }

    current_node_ = GetBestChild(current_node_);
    return current_node_->GetLastMove();
}

template <typename State, typename Move, typename UCB1Policy>
Move MCTS<State, Move, UCB1Policy>::ParallelSearch() {
    // Leaf Parallelisation
#if EANBE_PPL && defined(_WIN32)
    concurrency::parallel_for(0, evaluate_count_,
#else
    ParallelFor(evaluate_count_,
#endif
    [this](int32_t) {
        node_ptr_type selected_parent;
        node_ptr_type selected_leaf;
        {
            std::lock_guard<std::mutex> guard{ root_mutex_ };
            selected_parent = Select();
            selected_leaf = Expand(selected_parent);
        }
        auto score = Rollout(selected_leaf);
        std::lock_guard<std::mutex> guard{ root_mutex_ };
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
        auto itr = std::find_if(children.begin(), children.end(),[&opponent_move](
                                const typename MCTS<State, Move, UCB1Policy>::node_ptr_type& node) {
            return node->GetLastMove() == opponent_move;
        });
        if (itr != children.end()) {
            current_node_ = *itr;
        }
    }
}

template <typename State, typename Move, typename UCB1Policy>
inline typename MCTS<State, Move, UCB1Policy>::node_ptr_type MCTS<State, Move, UCB1Policy>::Select() const {
    auto selected_node = current_node_;
    while (selected_node->HasPassibleMoves() && selected_node->IsLeaf()) {
        selected_node = GetBestUCBChild(selected_node);
    }
    return selected_node;
}

template <typename State, typename Move, typename UCB1Policy>
inline typename MCTS<State, Move, UCB1Policy>::node_ptr_type MCTS<State, Move, UCB1Policy>::Expand(typename MCTS<State, Move, UCB1Policy>::node_ptr_type& parent) {
    if (!parent->HasPassibleMoves()) {
        const auto& available_moves = parent->GetMoves();
        auto idx = RNG::Get()(size_t(0), available_moves.size() - 1);
        return parent->MakeChild(available_moves[idx]);
    }
    else if (parent->IsLeaf()) {
        const auto& children = parent->GetChildren();
        auto idx = RNG::Get()(size_t(0), children.size() - 1);
        return children[idx];
    }
    return parent;
}

template <typename State, typename Move, typename UCB1Policy>
inline double MCTS<State, Move, UCB1Policy>::Rollout(const typename MCTS<State, Move, UCB1Policy>::node_ptr_type& leaf) {
    double total_score = 0.0;
    for (auto i = 0; i < rollout_limit_; ++i) {
        auto state = leaf->GetState();
        // Simulation
        while (!state.IsTerminal()) {
            state.ApplyMove(state.GetRandomMove());
        }
        double result = state.Evaluate();
        result = 0.5 * (result + 1) * (current_node_->GetPlayerID() == 1)
                + 0.5 * (1 - result) * (current_node_->GetPlayerID() == 2);
        total_score += result;
    }
    return total_score;
}

template <typename State, typename Move, typename UCB1Policy>
inline void MCTS<State, Move, UCB1Policy>::BackPropagation(
        const typename MCTS<State, Move, UCB1Policy>::node_ptr_type& leaf,
        double score) {
    leaf->Update(score);
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
        parent->Update(score);
    }
}

}

