#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <random>
#include <cmath>

namespace mcts {

class RNG {
public:
	static RNG& Get() {
		static thread_local RNG rng;
		return rng;
	}

	template <typename T>
	T operator()(T min, T max) noexcept {
        return std::uniform_int_distribution<T>(min, max)(engine_);
	}

private:
	RNG() noexcept {
        std::random_device random_device;
        std::vector<std::random_device::result_type> seed_data(std::mt19937_64::state_size);
        std::generate(std::begin(seed_data),
            std::end(seed_data),
            std::ref(random_device));
        std::seed_seq seq(std::begin(seed_data),
            std::end(seed_data));
        engine_.seed(seq);
	}

	std::mt19937_64 engine_;
};

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

	Node(const State& state = State(), const Move& move = Move(), NodePtr<State, Move> parent = nullptr)
		: state_(state)
		, move_(move)
		, player_id_(state.GetPlayerID())
		, wins_(0)
		, visits_(0)
		, parent_(parent)
		, possible_moves_(state.GetLegalMoves()) {
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
		wins_ += result;
		++visits_;
	}

	void Update(double result, size_t visits) noexcept {
		wins_ += result;
		visits_ += visits;
	}

	bool HasPassibleMoves() const noexcept {
		return possible_moves_.empty();
	}

	double GetWins() const noexcept {
		return wins_;
	}

	size_t GetVisits() const noexcept {
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

    const std::vector<ptr_type>& GetChildren() const {
        return children_;
    }

    ptr_type GetParent() {
        return parent_.lock();
	}

private:
	void RemoveMove(const Move& move) {
		auto itr = std::find(possible_moves_.begin(), possible_moves_.end(), move);
		if (itr != possible_moves_.end()) {
			possible_moves_.erase(itr);
		}
	}

	State state_;
	Move move_;
	int8_t player_id_;
	double wins_;
	size_t visits_;
    parent_ptr_type parent_;
    std::vector<ptr_type> children_;
	std::vector<Move> possible_moves_;
};

template <typename State, typename Move>
class MCTS {
public:
    using node_ptr_type = typename Node<State, Move>::ptr_type;

	MCTS();

    Move Search(int evaluate_count);
	
	void SetOpponentMove(const Move & opponent_move);

private:
	constexpr double DefaultUCB() const {
		return std::sqrt(2.0);
	}

	double GetUCB(const node_ptr_type &node) const;

	node_ptr_type GetBestChild(const node_ptr_type &parent) const;

	node_ptr_type Select() const;

	node_ptr_type Expand(node_ptr_type &node);

    double Rollout(int evaluate_count, const node_ptr_type &leaf);

	void BackPropagation(const node_ptr_type &leaf, double score);

    int visits_;
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
	return (node->GetWins() / node->GetVisits())
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
Move MCTS<State, Move>::Search(int evaluate_count) {
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
			return ((node->GetWins() / double(node->GetVisits())) > (large->GetWins() / double(large->GetVisits())));
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
double MCTS<State, Move>::Rollout(int evaluate_count, const typename MCTS<State, Move>::node_ptr_type& leaf) {
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

