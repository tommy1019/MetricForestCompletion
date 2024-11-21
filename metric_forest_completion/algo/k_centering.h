#pragma once

#include <algorithm>
#include <vector>

#include "clustering.h"

// Implementation for the k-centering clustering algorithm

template <typename T, typename F>
Clustering k_centering(const std::vector<T>& points, size_t num_clusters, size_t inital_index, F dist_func) {
    auto start = std::chrono::high_resolution_clock::now();

    if (points.size() < num_clusters)
        abort();

    if (num_clusters <= 1) {
        return Clustering{.assignments = std::vector<size_t>(points.size(), 0), .runtime = 0};
    }

    auto closest_point = [&](T p, const std::vector<T>& points) {
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

    auto furthest_point = [&](T p, const std::vector<T>& points) {
        size_t res = 0;
        float dist = dist_func(p, points[0]);

        for (size_t i = 1; i < points.size(); i++) {
            auto cur_dist = dist_func(p, points[i]);
            if (cur_dist > dist) {
                dist = cur_dist;
                res = i;
            }
        }

        return res;
    };

    using DistType = decltype(dist_func(points[0], points[0]));

    std::vector<T> centroids;
    centroids.reserve(num_clusters);
    centroids.push_back(points[inital_index]);

    std::vector<DistType> cur_distances;
    cur_distances.reserve(points.size());

    size_t second_index = 0;
    auto max_dist = dist_func(points[0], centroids[0]);

    for (size_t i = 0; i < points.size(); i++) {
        auto dist = dist_func(points[i], centroids[0]);
        if (dist > max_dist) {
            second_index = i;
            max_dist = dist;
        }
        cur_distances.push_back(dist);
    }
    centroids.push_back(points[second_index]);

    while (centroids.size() < num_clusters) {
        size_t new_index = 0;
        auto max_dist = std::min(dist_func(points[0], centroids.back()), cur_distances[0]);
        cur_distances[0] = max_dist;

        for (size_t i = 1; i < points.size(); i++) {
            auto dist = dist_func(points[i], centroids.back());
            if (dist < cur_distances[i])
                cur_distances[i] = dist;

            if (cur_distances[i] > max_dist) {
                max_dist = cur_distances[i];
                new_index = i;
            }
        }

        centroids.push_back(points[new_index]);
    }

    std::vector<size_t> res;
    res.reserve(points.size());
    for (size_t i = 0; i < points.size(); i++) {
        res.push_back(closest_point(points[i], centroids));
    }

    auto end = std::chrono::high_resolution_clock::now();

    return Clustering{.assignments = res, .runtime = std::chrono::duration<double, std::milli>(end - start).count()};
}

template <typename T, typename F>
Clustering k_centering(const std::vector<T>& points, size_t num_clusters, F dist_func) {
    return k_centering(points, num_clusters, points.size() / 2, dist_func);
}
