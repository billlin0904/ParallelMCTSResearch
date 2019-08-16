#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <random>
#include <cmath>
#include <ostream>

#include "rng.h"
#include "node.h"

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>

namespace mcts {

using namespace rapidjson;

template <typename State, typename Move>
class MCTS {
public:
    using node_ptr_type = typename Node<State, Move>::ptr_type;

    MCTS();

	MCTS(const MCTS&) = delete;
	MCTS& operator=(const MCTS &) = delete;

    Move Search(int32_t evaluate_count);

    void SetOpponentMove(const Move & opponent_move);

private:
    constexpr double DefaultUCB() const noexcept {
        return std::sqrt(2.0);
    }

    double GetUCB(const node_ptr_type &node) const;

    node_ptr_type GetBestChild(const node_ptr_type &parent) const;

    node_ptr_type Select() const;

    node_ptr_type Expand(node_ptr_type &node);

    double Rollout(int32_t evaluate_count, const node_ptr_type &leaf);

    void BackPropagation(const node_ptr_type &leaf, double score);

	friend void WriteChildren(const MCTS& mcts, Value & parent_node, node_ptr_type parent, Document & document) {
		Value children_node(rapidjson::kArrayType);

		for (const auto &children : parent->GetChildren()) {
			Value object(kObjectType);
			object.AddMember("visits", children->GetVisits(), document.GetAllocator());
			object.AddMember("score", children->GetScore(), document.GetAllocator());
			object.AddMember("state", children->GetState().ToString(), document.GetAllocator());
			object.AddMember("move", children->GetLastMove().ToString(), document.GetAllocator());
			if (children->IsLeaf()) {
				WriteChildren(mcts, object, children, document);
			}
			children_node.PushBack(object, document.GetAllocator());
		}
		parent_node.AddMember("children", children_node, document.GetAllocator());
	}

	friend std::ostream& operator<<(std::ostream& stream, const MCTS& mcts) {		
		Document document;

		Value parent_node(kObjectType);
		parent_node.AddMember("visits", mcts.root_->GetVisits(), document.GetAllocator());
		parent_node.AddMember("score", mcts.root_->GetScore(), document.GetAllocator());
		parent_node.AddMember("state", mcts.root_->GetState().ToString(), document.GetAllocator());
		parent_node.AddMember("move", mcts.root_->GetLastMove().ToString(), document.GetAllocator());

		WriteChildren(mcts, parent_node, mcts.root_, document);
		document.SetObject();
		document.AddMember("result", parent_node, document.GetAllocator());

		OStreamWrapper wrapper{ stream };
		PrettyWriter<OStreamWrapper> writer{ wrapper };
		document.Accept(writer);
		return stream;
	}

    int32_t visits_;
    node_ptr_type root_;
    node_ptr_type current_node_;
};

template <typename State, typename Move>
MCTS<State, Move>::MCTS()
    : visits_(0)
    , root_(std::make_shared<Node<State, Move>>())
    , current_node_(root_) {
}

template <typename State, typename Move>
double MCTS<State, Move>::GetUCB(const typename MCTS<State, Move>::node_ptr_type& node) const {
    return (node->GetScore() / node->GetVisits())
            + DefaultUCB() * std::sqrt(std::log(double(node->GetVisits())))
            / double(node->GetVisits());

}

template <typename State, typename Move>
typename MCTS<State, Move>::node_ptr_type MCTS<State, Move>::GetBestChild(const typename MCTS<State, Move>::node_ptr_type& parent) const {
    const auto& children = parent->GetChildren();
    auto itr = std::max_element(children.begin(), children.end(), [this](
                                const typename MCTS<State, Move>::node_ptr_type &first,
                                const typename MCTS<State, Move>::node_ptr_type& last) {
        return GetUCB(first) > GetUCB(last);
    });
    return *itr;
}

template <typename State, typename Move>
Move MCTS<State, Move>::Search(int32_t evaluate_count) {
    visits_ = evaluate_count;

    for (auto i = 0; i < evaluate_count; ++i) {
        auto selected_parent = Select();
        auto selected_leaf = Expand(selected_parent);
        auto score = Rollout(evaluate_count, selected_leaf);
        BackPropagation(selected_leaf, score);
    }

    // Select best move
    auto candidate_node = current_node_->GetChildren();
    auto itr = std::max_element(candidate_node.begin(), candidate_node.end(),
                                [](const typename MCTS<State, Move>::node_ptr_type& large, const typename MCTS<State, Move>::node_ptr_type& node) {
        return ((node->GetScore() / double(node->GetVisits())) > (large->GetScore() / double(large->GetVisits())));
    });
    current_node_ = *itr;
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
typename MCTS<State, Move>::node_ptr_type MCTS<State, Move>::Select() const {
    auto selected_node = current_node_;
    while (selected_node->HasPassibleMoves() && selected_node->IsLeaf()) {
        selected_node = GetBestChild(selected_node);
    }
    return selected_node;
}

template <typename State, typename Move>
typename MCTS<State, Move>::node_ptr_type MCTS<State, Move>::Expand(typename MCTS<State, Move>::node_ptr_type& parent) {
    if (!(parent->HasPassibleMoves())) {
        const auto &available_moves = parent->GetMoves();
        auto idx = RNG::Get()(0, int32_t(available_moves.size() - 1));
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
double MCTS<State, Move>::Rollout(int32_t evaluate_count, const typename MCTS<State, Move>::node_ptr_type& leaf) {
    double total_score = 0.0;
    for (auto i = 0; i < evaluate_count; ++i) {
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
void MCTS<State, Move>::BackPropagation(const typename MCTS<State, Move>::node_ptr_type& leaf, double score) {
    leaf->Update(score, visits_);
    auto parent = leaf->GetParent();
    while (parent != nullptr) {
        parent->Update(score, visits_);
        parent = parent->GetParent();
    }
}

}

