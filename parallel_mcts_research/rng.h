#pragma once

#include <random>
#include <algorithm>
#include <functional>

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

}
