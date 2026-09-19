#pragma once

#include "FlameGenome.hpp"
#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace ApoNeo::Core {

class RandomFlameGenerator {
public:
    explicit RandomFlameGenerator(uint64_t seed = 0);

    /// @brief Generate a single unique, aesthetically balanced FlameGenome
    FlameGenome generate(const std::string& name = "Random Flame");

    /// @brief Generate a batch of N diverse random flames
    std::vector<FlameGenome> generate_batch(size_t count, const std::string& base_name = "Random");

    /// @brief Generate a vibrant procedural palette
    static Palette generate_random_palette(std::mt19937_64& rng);

    /// @brief Set random seed
    void set_seed(uint64_t seed);

private:
    std::mt19937_64 m_rng;
};

} // namespace ApoNeo::Core
