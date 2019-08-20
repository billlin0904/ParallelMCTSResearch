#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <random>
#include <cmath>
#include <ostream>

#include "rng.h"
#include "node.h"

#define ENABLE_JSON 1

#if ENABLE_JSON
#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
using namespace rapidjson;
#endif

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

    void SetOpponentMove(const Move& opponent_move);

	size_t GetMaxDepth(size_t parent_depth = 0) const;

	node_ptr_type GetCurrentNode() const;

#if ENABLE_JSON
    void WriteTo(Document& document) const;
#endif

private:
    constexpr double DefaultUCB() const noexcept {
        return std::sqrt(2.0);
    }

    double GetUCB(const node_ptr_type& node) const;

	node_ptr_type GetBestChild(const node_ptr_type& parent) const;

    node_ptr_type GetBestUCBChild(const node_ptr_type& parent) const;

    node_ptr_type Select() const;

    node_ptr_type Expand(node_ptr_type& node);

    double Rollout(int32_t evaluate_count, const node_ptr_type& leaf);

    void BackPropagation(const node_ptr_type& leaf, double score);
#if ENABLE_JSON
	friend void WriteChildren(const MCTS& mcts, Value & parent_node, node_ptr_type parent, Document& document) {
		Value children_node(rapidjson::kArrayType);

		for (const auto &children : parent->GetChildren()) {
			Value object(kObjectType);
			object.AddMember("visits", children->GetVisits(), document.GetAllocator());
            object.AddMember("value", children->GetScore(), document.GetAllocator());
            object.AddMember("state", children->GetState().ToString(), document.GetAllocator());
            object.AddMember("name", children->GetLastMove().ToString(), document.GetAllocator());
			if (children->IsLeaf()) {
				WriteChildren(mcts, object, children, document);
			}
			children_node.PushBack(object, document.GetAllocator());
		}
		parent_node.AddMember("children", children_node, document.GetAllocator());
	}    

	friend std::ostream& operator<<(std::ostream& stream, const MCTS& mcts) {		
        Document document;
        document.SetObject();
        mcts.WriteTo(document);
		OStreamWrapper wrapper{ stream };
		PrettyWriter<OStreamWrapper> writer{ wrapper };
		document.Accept(writer);
		return stream;
	}
#endif
	int64_t visits_;
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
MCTS<State, Move>::MCTS(const children_vector_type &children)
	: MCTS() {
	for (auto child : children) {
		current_node_->AddChild(child);
	}
}

#if ENABLE_JSON
template <typename State, typename Move>
void MCTS<State, Move>::WriteTo(Document& document) const {
    Value parent_node(kObjectType);

    parent_node.AddMember("visits", root_->GetVisits(), document.GetAllocator());
    parent_node.AddMember("value", root_->GetScore(), document.GetAllocator());
    parent_node.AddMember("name", root_->GetLastMove().ToString(), document.GetAllocator());
    parent_node.AddMember("max_depth", GetMaxDepth(), document.GetAllocator());
    parent_node.AddMember("state", root_->GetState().ToString(), document.GetAllocator());

    WriteChildren(*this, parent_node, root_, document);
    document.AddMember("mcts_result", parent_node, document.GetAllocator());
}
#endif

template <typename State, typename Move>
double MCTS<State, Move>::GetUCB(const typename MCTS<State, Move>::node_ptr_type& node) const {
    return (node->GetScore() / node->GetVisits())
            + DefaultUCB() * std::sqrt(std::log(double(node->GetVisits())))
            / double(node->GetVisits());

}

template <typename State, typename Move>
typename MCTS<State, Move>::node_ptr_type MCTS<State, Move>::GetBestUCBChild(const typename MCTS<State, Move>::node_ptr_type& parent) const {
    const auto& children = parent->GetChildren();
    auto itr = std::max_element(children.begin(), children.end(), [this](
                                const typename MCTS<State, Move>::node_ptr_type &first,
                                const typename MCTS<State, Move>::node_ptr_type& last) {
        return GetUCB(first) > GetUCB(last);
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
size_t MCTS<State, Move>::GetMaxDepth(size_t parent_depth) const {
	return root_->GetMaxDepth(parent_depth);
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
        selected_node = GetBestUCBChild(selected_node);
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
double MCTS<State, Move>::Rollout(int32_t rollout_limit, const typename MCTS<State, Move>::node_ptr_type& leaf) {
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
void MCTS<State, Move>::BackPropagation(const typename MCTS<State, Move>::node_ptr_type& leaf, double score) {
    leaf->Update(score, visits_);
    auto parent = leaf->GetParent();
    while (parent != nullptr) {
        parent->Update(score, visits_);
        parent = parent->GetParent();
    }
}

}

