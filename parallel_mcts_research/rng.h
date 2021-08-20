// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <random>

namespace mcts {

class RNG {
public:
	static RNG& Get();

    template<class T, std::enable_if_t<std::is_integral<T>::value>* = nullptr>
    T operator()(T min, T max) noexcept {
        return std::uniform_int_distribution(min, max)(engine_);
    }

private:
	RNG() noexcept;

    std::mt19937_64 engine_;
};

}
