#pragma once

#include <algorithm>
#include <vector>

// Computes OPT MST for a given graph. Edges are assumed to be a struct that contains a and b elements that are integers and a weight element that is a float. Nodes are assumed to be 0 indexed

template <typename T>
concept EdgeType = requires(T t) {
    { t.weight } -> std::convertible_to<float>;
    { t.a } -> std::convertible_to<size_t>;
    { t.b } -> std::convertible_to<size_t>;
};

template <EdgeType T>
std::vector<T> MST(size_t num_nodes, std::vector<T> edges) {
    std::sort(edges.begin(), edges.end(), [](T& e1, T& e2) { return e1.weight < e2.weight; });

    std::vector<size_t> sets;
    sets.reserve(num_nodes);
    for (size_t i = 0; i < num_nodes; i++)
        sets.push_back(i);

    std::vector<T> res;
    res.reserve(num_nodes - 1);

    for (auto& e : edges) {
        if (sets[e.a] == sets[e.b]) {
            continue;
        }

        auto old_set = sets[e.b];
        auto new_set = sets[e.a];

        res.push_back(e);

        for (auto& s : sets)
            if (s == old_set)
                s = new_set;
    }

    return res;
}