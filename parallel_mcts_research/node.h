#pragma once

#include <memory>
#include <vector>
#include <algorithm>
#include <cmath>

namespace mcts {

template <typename State, typename Move, typename UCB1Policy>
class Node;

template <typename State, typename Move, typename UCB1Policy>
using NodePtr = std::shared_ptr<Node<State, Move, UCB1Policy>>;

template <typename State, typename Move, typename UCB1Policy>
using WeakPtr = std::weak_ptr<Node<State, Move, UCB1Policy>>;

struct EBO { };

// Upper Confidence Bounds
struct DefaultUCB1Policy : public EBO {
	double operator()(double score, int64_t visits) const {
		return (score / visits)
			+ DefaultConstant() * std::sqrt(std::log(double(visits)))
			/ double(visits);
	}

	static double DefaultConstant() noexcept {
		static const auto c = std::sqrt(2.0);
		return c;
	}
};

// UCB1Tuned
struct UCB1Tuned : public EBO {
	double operator()(double score, int64_t visits) const {
		static const auto MAX_BERNOULLI_RANDOM_VARIABLE_VARIANCE = 0.25;
		double variance = MAX_BERNOULLI_RANDOM_VARIABLE_VARIANCE;
		return (score / visits)
			+ std::sqrt(std::log(visits) / visits)
			* (std::min)(MAX_BERNOULLI_RANDOM_VARIABLE_VARIANCE,
				variance + std::sqrt(2.0 * std::log(visits) / visits));
	}	
};

template <typename State, typename Move, typename UCB1Policy>
class Node : public std::enable_shared_from_this<Node<State, Move, UCB1Policy>> {
public:
    using self_type = Node<State, Move, UCB1Policy>;
    using ptr_type = NodePtr<State, Move, UCB1Policy>;
    using parent_ptr_type = WeakPtr<State, Move, UCB1Policy>;

    Node(const State& state = State(),
         const Move& move = Move(),
         NodePtr<State, Move, UCB1Policy> parent = nullptr)
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
		return ucb1_policy_(GetScore(), GetVisits());
    }

private:
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
	UCB1Policy ucb1_policy_;
    std::vector<ptr_type> children_;
    std::vector<Move> possible_moves_;
};

}
