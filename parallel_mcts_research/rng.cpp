#include "rng.h"

namespace mcts {

FastMutex RNG::mutex_;

RNG& RNG::Get() {
	static RNG rng;
	return rng;
}

RNG::RNG() noexcept {
	std::random_device random_device;
	std::vector<std::random_device::result_type> seed_data(std::mt19937_64::state_size);
	std::generate(std::begin(seed_data),
		std::end(seed_data),
		std::ref(random_device));
	std::seed_seq seq(std::begin(seed_data),
		std::end(seed_data));
	engine_.seed(seq);
}

}
