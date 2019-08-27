#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <ostream>

#include "rng.h"
#include "node.h"
#include "threadpool.h"

namespace mcts {

template <typename State, typename Move>
class MCTS {
public:
    using node_ptr_type = typename Node<State, Move>::ptr_type;
    using children_vector_type = std::vector<node_ptr_type>;

    MCTS();

    MCTS(const children_vector_type& children);

    MCTS(const MCTS&) = default;
    MCTS& operator=(const MCTS &) = default;

    Move Search(int32_t evaluate_count, int32_t rollout_limit);

    Move ParallelSearch(int32_t evaluate_count, int32_t rollout_limit);

    void SetOpponentMove(const Move& opponent_move);

    node_ptr_type GetCurrentNode() const;

    node_ptr_type GetRoot() const;

private:
    node_ptr_type GetBestChild(const node_ptr_type& parent) const;

    node_ptr_type GetBestUCBChild(const node_ptr_type& parent) const;

    node_ptr_type Select() const;

    node_ptr_type Expand(node_ptr_type& node);

    double Rollout(int32_t evaluate_count, const node_ptr_type& leaf);

    void BackPropagation(const node_ptr_type& leaf, double score);

    std::atomic<int32_t> max_depth_;
    int64_t visits_;
    node_ptr_type root_;
    node_ptr_type current_node_;
};

template <typename State, typename Move>
MCTS<State, Move>::MCTS()
    : max_depth_(0)
    , visits_(0)
    , root_(std::make_shared<Node<State, Move>>())
    , current_node_(root_) {
}

template <typename State, typename Move>
MCTS<State, Move>::MCTS(const children_vector_type &children)
    : MCTS() {
    for (auto child : children) {
        current_node_->AddChild(child);
    }
}

template <typename State, typename Move>
typename MCTS<State, Move>::node_ptr_type MCTS<State, Move>::GetRoot() const {
    return root_;
}

template <typename State, typename Move>
typename MCTS<State, Move>::node_ptr_type MCTS<State, Move>::GetBestUCBChild(const typename MCTS<State, Move>::node_ptr_type& parent) const {
    const auto& children = parent->GetChildren();
    auto itr = std::max_element(children.begin(), children.end(), [this](
                                const typename MCTS<State, Move>::node_ptr_type &first,
                                const typename MCTS<State, Move>::node_ptr_type& last) {
        return first->GetUCB() > last->GetUCB();
    });
    return *itr;
}

template <typename State, typename Move>
typename MCTS<State, Move>::node_ptr_type MCTS<State, Move>::GetBestChild(const typename MCTS<State, Move>::node_ptr_type& parent) const {
    auto candidate_node = parent->GetChildren();
    auto itr = std::max_element(candidate_node.begin(), candidate_node.end(),
                                [](const typename MCTS<State, Move>::node_ptr_type& large, const typename MCTS<State, Move>::node_ptr_type& node) {
        return ((node->GetScore() / double(node->GetVisits())) > (large->GetScore() / double(large->GetVisits())));
    });
    return *itr;
}

template <typename State, typename Move>
typename MCTS<State, Move>::node_ptr_type MCTS<State, Move>::GetCurrentNode() const {
    return current_node_;
}

template <typename State, typename Move>
Move MCTS<State, Move>::Search(int32_t evaluate_count, int32_t rollout_limit) {
    visits_ = evaluate_count;
    max_depth_ = 0;

    for (auto i = 0; i < evaluate_count; ++i) {
        auto selected_parent = Select();
        auto selected_leaf = Expand(selected_parent);
        auto score = Rollout(rollout_limit, selected_leaf);
        BackPropagation(selected_leaf, score);
    }

    current_node_ = GetBestChild(current_node_);
    return current_node_->GetLastMove();
}

template <typename State, typename Move>
Move MCTS<State, Move>::ParallelSearch(int32_t evaluate_count, int32_t rollout_limit) {
    visits_ = evaluate_count;
    max_depth_ = 0;

    std::mutex root_mutex;
    auto tasks = ParallelFor(evaluate_count, [&root_mutex, rollout_limit, this](int32_t) {
        node_ptr_type selected_leaf;
        {
            std::lock_guard<std::mutex> guard{ root_mutex };
            auto selected_parent = Select();
            selected_leaf = Expand(selected_parent);
        }
        auto score = Rollout(rollout_limit, selected_leaf);
        {
            std::lock_guard<std::mutex> guard{ root_mutex };
            BackPropagation(selected_leaf, score);
        }
    });
    for (auto& task : tasks) {
        task.get();
    }

    current_node_ = GetBestChild(current_node_);
    return current_node_->GetLastMove();
}

template <typename State, typename Move>
void MCTS<State, Move>::SetOpponentMove(const Move& opponent_move) {
    const auto& available_moves = current_node_->GetMoves();
    auto itr = std::find(available_moves.cbegin(),
                         available_moves.cend(),
                         opponent_move);
    if (itr != available_moves.end()) {
        current_node_ = current_node_->MakeChild(opponent_move);
    }
    else {
        const auto & children = current_node_->GetChildren();
        auto itr = std::find_if(children.begin(), children.end(),
                                [&opponent_move](const typename MCTS<State, Move>::node_ptr_type& node) {
            return node->GetLastMove() == opponent_move;
        });
        if (itr != children.end()) {
            current_node_ = *itr;
        }
    }
}

template <typename State, typename Move>
inline typename MCTS<State, Move>::node_ptr_type MCTS<State, Move>::Select() const {
    auto selected_node = current_node_;
    while (selected_node->HasPassibleMoves() && selected_node->IsLeaf()) {
        selected_node = GetBestUCBChild(selected_node);
    }
    return selected_node;
}

template <typename State, typename Move>
inline typename MCTS<State, Move>::node_ptr_type MCTS<State, Move>::Expand(typename MCTS<State, Move>::node_ptr_type& parent) {
    if (!parent->HasPassibleMoves()) {
        const auto &available_moves = parent->GetMoves();
        auto idx = RNG::Get()(0, int32_t(available_moves.size() - 1));
        ++max_depth_;
        return parent->MakeChild(available_moves[idx]);
    }
    else if (parent->IsLeaf()) {
        const auto& children = parent->GetChildren();
        auto idx = RNG::Get()(0, int32_t(children.size() - 1));
        return children[idx];
    }
    return parent;
}

template <typename State, typename Move>
inline double MCTS<State, Move>::Rollout(int32_t rollout_limit, const typename MCTS<State, Move>::node_ptr_type& leaf) {
    double total_score = 0.0;
    for (auto i = 0; i < rollout_limit; ++i) {
        auto state = leaf->GetState();
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

template <typename State, typename Move>
inline void MCTS<State, Move>::BackPropagation(const typename MCTS<State, Move>::node_ptr_type& leaf, double score) {
    leaf->Update(score, visits_);
    auto parent = leaf->GetParent();
#if 0
    while (parent != nullptr) {
        parent->Update(score, visits_);
        parent = parent->GetParent();
    }
#else
    if (!parent) {
        return;
    }

    std::vector<node_ptr_type> parents;
    parents.reserve(max_depth_);
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
        parent->Update(score, visits_);
    }
#endif
}

}

