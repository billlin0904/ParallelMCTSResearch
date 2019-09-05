// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <cstdint>
#include <algorithm>
#include <cmath>

namespace mcts {

// Upper Confidence Bounds
class DefaultUCB1Policy {
public:
    DefaultUCB1Policy()
        : score_(0)
        , visits_(0) {
    }

    double GetScore() const noexcept {
        return score_;
    }

    int64_t GetVisits() const noexcept {
        return visits_;
    }

	void Update(double score) noexcept {
		score_ += score;
		++visits_;
	}

    void Update(double score, int64_t visits) noexcept {
        score_ += score;
		visits_ += visits;
    }

    inline double operator()(double parent_visits) const {
        return (score_ / visits_)
                + DefaultConstant() * std::sqrt(std::log(parent_visits))
                / double(visits_);
    }
private:
    static double DefaultConstant() noexcept {
        static const auto c = std::sqrt(2.0);
        return c;
    }
    double score_;
    int64_t visits_;
};

// Upper Confidence Bounds Tuned
class UCB1TunedPolicy {
public:
    UCB1TunedPolicy()
        : score_(0)
        , visits_(0)
        , sqrt_score_(0) {
    }

    double GetScore() const noexcept {
        return score_;
    }

    int64_t GetVisits() const noexcept {
        return visits_;
    }

    void Update(double score) noexcept {
        score_ += score;
        sqrt_score_ += sqrt_score_ * sqrt_score_;
        ++visits_;
    }

	void Update(double score, int64_t visits) noexcept {
		score_ += score;
		sqrt_score_ += sqrt_score_ * sqrt_score_;
		visits_ += visits;
	}

    inline double operator()(double parent_visits) const {
        static const auto MAX_BERNOULLI_RANDOM_VARIABLE_VARIANCE = 0.25;
        return (score_ / visits_)
                + std::sqrt(std::log(parent_visits) / visits_)
                * std::min(MAX_BERNOULLI_RANDOM_VARIABLE_VARIANCE,
                    sqrt_score_ / visits_ - std::pow(sqrt_score_ / visits_, 2) + std::sqrt(2 * std::log(parent_visits) / visits_));
    }

private:
    double score_;
    int64_t visits_;
    double sqrt_score_;
};

}
