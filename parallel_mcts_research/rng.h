// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <random>
#include <algorithm>
#include <functional>

namespace mcts {

class RNG {
public:
	static RNG& Get();

    template <typename T>
    inline T operator()(T min, T max) noexcept {
        return std::uniform_int_distribution<T>(min, max)(engine_);
    }

private:
	RNG() noexcept;

    std::mt19937_64 engine_;
};

}
