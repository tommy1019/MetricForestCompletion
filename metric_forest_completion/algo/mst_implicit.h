#pragma once

#include "mst.h"

// Computes OPT MST for an implciit graph. Takes only a list of points and a distance function, returns a list of edges with their distances pre-calculated

struct WeightedEdge {
    float weight;
    size_t a;
    size_t b;
};

template <typename T, typename F>
std::vector<WeightedEdge> MST_Implicit(std::vector<T> points, F dist_func) {
    if (points.size() < 2)
        return {};

    std::vector<WeightedEdge> all_edges;
    for (size_t i = 0; i < points.size() - 1; i++) {
        for (size_t j = i + 1; j < points.size(); j++) {
            all_edges.push_back({dist_func(points[i], points[j]), i, j});
        }
    }

    return MST(points.size(), all_edges);
}