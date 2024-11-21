#pragma once

#include <ranges>
#include <vector>

#include "mst_implicit.h"

// Runs the MFC algorithm. Takes a list of points and a clustering as input, runs optimal MST on the clusters and completes the MST approximation for the entire list of points

struct MetricForestCompletion {
    std::vector<std::vector<WeightedEdge>> cluster_edges;
    std::vector<WeightedEdge> completion_edges;

    double sub_cluster_runtime;
    double completion_edges_runtime;
    double completion_runtime;
};

template <typename T, typename DistFunc>
MetricForestCompletion metric_forest_completion(std::vector<T> points, size_t cluster_count, std::vector<size_t> cluster_assignments, DistFunc dist_func) {

    constexpr static auto time_code = [](auto f) {
        auto start = std::chrono::high_resolution_clock::now();
        f();
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start).count();
    };

    std::vector<std::vector<size_t>> cluster_vecs;
    cluster_vecs.resize(cluster_count);

    std::vector<std::vector<WeightedEdge>> cluster_msts;
    cluster_msts.resize(cluster_count);

    for (size_t i = 0; i < points.size(); i++) {
        auto assign = cluster_assignments[i];
        cluster_vecs[assign].push_back(i);
    }

    auto sub_cluster_runtime = time_code([&]() {
        for (size_t i = 0; i < cluster_count; i++) {
            cluster_msts[i] = [&]() {
                auto res = MST_Implicit(cluster_vecs[i], [&](size_t a, size_t b) { return dist_func(points[a], points[b]); });
                for (auto& e : res) {
                    e.a = cluster_vecs[i][e.a];
                    e.b = cluster_vecs[i][e.b];
                }
                return res;
            }();
        }
    });

    struct CompletionEdge {
        size_t a;
        size_t b;

        size_t a_rep;
        size_t b_rep;

        float weight;
    };

    std::vector<CompletionEdge> unmapped_completion_edges;
    unmapped_completion_edges.reserve(cluster_count * (cluster_count - 1));

    auto completion_edges_runtime = time_code([&]() {
        for (size_t i = 0; i < cluster_count - 1; i++) {
            for (size_t j = 1; j < cluster_count; j++) {

                if (cluster_vecs[i].size() == 0 || cluster_vecs[j].size() == 0)
                    continue;

                size_t i_rep = 0;
                size_t j_rep = 0;

                CompletionEdge e;
                e.a = i;
                e.b = j;
                e.weight = INFINITY;

                for (size_t k = 0; k < cluster_vecs[j].size(); k++) {
                    auto dist = dist_func(points[cluster_vecs[i][i_rep]], points[cluster_vecs[j][k]]);
                    if (dist < e.weight) {
                        e.a_rep = cluster_vecs[i][i_rep];
                        e.b_rep = cluster_vecs[j][k];
                        e.weight = dist;
                    }
                }

                for (size_t k = 0; k < cluster_vecs[i].size(); k++) {
                    auto dist = dist_func(points[cluster_vecs[i][k]], points[cluster_vecs[j][j_rep]]);
                    if (dist < e.weight) {
                        e.a_rep = cluster_vecs[i][k];
                        e.b_rep = cluster_vecs[j][j_rep];
                        e.weight = dist;
                    }
                }

                unmapped_completion_edges.push_back(e);
            }
        }
    });

    std::vector<CompletionEdge> completion;
    auto completion_runtime = time_code([&]() { completion = MST(cluster_count, unmapped_completion_edges); });

    auto completion_edges = std::ranges::to<std::vector>(completion | std::views::transform([&](CompletionEdge e) {
                                                             return WeightedEdge{
                                                                 .weight = e.weight,
                                                                 .a = e.a_rep,
                                                                 .b = e.b_rep,
                                                             };
                                                         }));

    return {
        .cluster_edges = cluster_msts,
        .completion_edges = completion_edges,

        .sub_cluster_runtime = sub_cluster_runtime,
        .completion_edges_runtime = completion_edges_runtime,
        .completion_runtime = completion_runtime,
    };
}