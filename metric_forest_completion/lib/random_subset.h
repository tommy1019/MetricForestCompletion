#pragma once

#include <random>
#include <vector>

// https://en.wikipedia.org/wiki/Fisher–Yates_shuffle
template <typename T>
auto random_subset(std::vector<T> vec, size_t N, auto& random) {
    N = std::min(N, vec.size());

    for (size_t i = 0; i < N; i++) {
        std::uniform_int_distribution dist(i, vec.size() - 1);
        size_t j = dist(random);
        std::swap(vec[i], vec[j]);
    }

    std::vector<T> subset;
    for (size_t i = 0; i < N; i++) {
        subset.push_back(vec[i]);
    }
    return subset;
};