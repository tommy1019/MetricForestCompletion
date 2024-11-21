#pragma once

#include <algorithm>
#include <random>
#include <ranges>
#include <vector>

#include "clustering.h"

// Implementation for the k-means clustering algorithm

template <typename T, typename F>
Clustering k_means(std::vector<T> points, std::vector<T> inital_centroids, F dist_func) {
    auto start = std::chrono::high_resolution_clock::now();

    auto centroids = inital_centroids;
    auto new_centroids = std::vector<T>(centroids.size(), T{});
    auto new_centroids_count = std::vector<size_t>(centroids.size(), 0);

    int ite_count = 0;

    auto closest_point = [&](T p, std::vector<T> points) {
        size_t res = 0;
        float dist = dist_func(p, points[0]);

        for (size_t i = 1; i < points.size(); i++) {
            auto cur_dist = dist_func(p, points[i]);
            if (cur_dist < dist) {
                dist = cur_dist;
                res = i;
            }
        }

        return res;
    };

    while (true) {
        for (size_t i = 0; i < centroids.size(); i++) {
            new_centroids_count[i] = 0;
            new_centroids[i] = {};
        }

        for (size_t i = 0; i < points.size(); i++) {
            auto assign = closest_point(points[i], centroids);
            new_centroids[assign] = new_centroids[assign] + points[i];
            new_centroids_count[assign]++;
        }

        for (size_t i = 0; i < centroids.size(); i++) {
            if (new_centroids_count[i] == 0)
                continue;
            new_centroids[i] = new_centroids[i] / new_centroids_count[i];
        }

        if (new_centroids == centroids)
            break;

        ite_count++;
        if (ite_count > 1000000)
            break;

        centroids = new_centroids;
    }

    std::vector<size_t> res;
    res.reserve(points.size());
    for (auto p : points)
        res.push_back(closest_point(p, centroids));

    auto end = std::chrono::high_resolution_clock::now();

    return Clustering{.assignments = res, .runtime = std::chrono::duration<double, std::milli>(end - start).count()};
}

template <typename T, typename F>
Clustering k_means(std::vector<T> points, size_t k, F dist_func) {
    return k_means(points, std::ranges::to<std::vector>(std::ranges::views::take(points, k)), dist_func);
}