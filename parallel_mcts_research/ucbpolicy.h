// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <cstdint>
#include <algorithm>
#include <cmath>

namespace mcts {

// Upper Confidence Bounds
class DefaultUCB1Policy {
public:
    DefaultUCB1Policy() noexcept
        : score_(0)
        , visits_(0) {
    }

    [[nodiscard]] double GetScore() const noexcept {
        return score_;
    }

    [[nodiscard]] int64_t GetVisits() const noexcept {
        return visits_;
    }

    void Update(double score, int32_t visits) noexcept {
        score_ += score;
		visits_ += visits;
    }

    double operator()(int64_t total_visits) const {
    	if (total_visits == 0) {
            return (std::numeric_limits<double>::max)();
    	}
        return (score_ / static_cast<double>(visits_)
                + DefaultConstant() * std::sqrt(std::log(total_visits) / static_cast<double>(visits_)));
    }
private:
    static double DefaultConstant() noexcept {      
        return 1.41421;
    }
    double score_;
    int64_t visits_;
};

// Upper Confidence Bounds Tuned
class UCB1TunedPolicy {
public:
    UCB1TunedPolicy() noexcept
        : score_(0)
        , visits_(0)
        , sqrt_score_(0) {
    }

    [[nodiscard]] double GetScore() const noexcept {
        return score_;
    }

    [[nodiscard]] int64_t GetVisits() const noexcept {
        return visits_;
    }

	void Update(double score, int32_t visits) noexcept {
		score_ += score;
		sqrt_score_ += sqrt_score_ * sqrt_score_;
		visits_ += visits;
	}

    double operator()(double parent_visits) const {
        const auto MAX_BERNOULLI_RANDOM_VARIABLE_VARIANCE = 0.25;
        const auto V = sqrt_score_ / visits_ - std::pow(sqrt_score_ / visits_, 2) + std::sqrt(2 * std::log(parent_visits) / visits_);
        return (score_ / visits_)
                + std::sqrt(std::log(parent_visits) / visits_)
                * (std::min)(MAX_BERNOULLI_RANDOM_VARIABLE_VARIANCE, V);
    }

private:
    double score_;
    int64_t visits_;
    double sqrt_score_;
};

}
