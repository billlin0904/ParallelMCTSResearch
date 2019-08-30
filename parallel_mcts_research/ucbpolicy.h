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

	inline double operator()(int32_t evaluate_count) const {
		return (score_ / visits_)
			+ DefaultConstant() * std::sqrt(std::log(double(evaluate_count)))
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

	inline double operator()(int32_t evaluate_count) const {
		static const auto MAX_BERNOULLI_RANDOM_VARIABLE_VARIANCE = 0.25;
		return -1;
	}

private:
	double score_;
	double sqrt_score_;
	int64_t visits_;
};

}