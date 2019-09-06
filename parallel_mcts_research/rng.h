// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <random>
#include <mutex>
#include <algorithm>
#include <functional>

namespace mcts {

class RNG {
public:
	static RNG& Get();

    template <typename T>
    inline T operator()(T min, T max) noexcept {
		static thread_local std::uniform_int_distribution<T> distribution;
		return distribution(engine_, decltype(distribution)::param_type{ min, max });
    }

private:
	RNG() noexcept;

    std::mt19937_64 engine_;
};

}
