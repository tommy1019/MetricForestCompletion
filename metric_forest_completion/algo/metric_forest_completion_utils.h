#pragma once

#include "mst_implicit.h"

#include <chrono>
#include <ranges>
#include <vector>

struct MetricForestCompletion {
    std::vector<std::vector<WeightedEdge>> cluster_edges;
    std::vector<WeightedEdge> completion_edges;

    size_t rep_count = 0;

    double sub_cluster_runtime = 0;
    double completion_edges_runtime = 0;
    double completion_runtime = 0;
    double find_reps_runtime = 0;
    double pick_reps_runtime = 0;
    double reps_cost = 0;

    size_t clustering_dist_calls = 0;
    size_t sub_cluster_dist_calls = 0;
    size_t mfc_dist_calls = 0;
};

struct CompletionEdge {
    size_t a;
    size_t b;

    size_t a_rep;
    size_t b_rep;

    float weight;
};

template <typename F>
double time_code(F f) {
    auto start = std::chrono::high_resolution_clock::now();
    f();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

template <typename F>
auto time_code_ret(F f) {
    auto start = std::chrono::high_resolution_clock::now();
    auto res = f();
    auto end = std::chrono::high_resolution_clock::now();
    return std::tuple<double, decltype(res)>{std::chrono::duration<double, std::milli>(end - start).count(), res};
}

template <typename T>
std::vector<std::vector<size_t>> create_cluster_vecs(size_t cluster_count, std::vector<T>& points, std::vector<size_t>& cluster_assignments) {
    std::vector<std::vector<size_t>> cluster_vecs;
    cluster_vecs.resize(cluster_count);

    for (size_t i = 0; i < points.size(); i++) {
        auto assign = cluster_assignments[i];
        cluster_vecs[assign].push_back(i);
    }

    return cluster_vecs;
};

template <typename T, typename F>
std::tuple<std::vector<std::vector<WeightedEdge>>, double> sub_clusters(size_t cluster_count, std::vector<T>& points, std::vector<std::vector<size_t>>& cluster_vecs, F& dist_func) {
    std::vector<std::vector<WeightedEdge>> cluster_msts;
    cluster_msts.resize(cluster_count);

    auto runtime = time_code([&]() {
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

    return std::make_tuple(cluster_msts, runtime);
};

std::tuple<std::vector<CompletionEdge>, double> get_completion(size_t cluster_count, std::vector<CompletionEdge> unmapped_completion_edges) {
    std::vector<CompletionEdge> completion;
    auto runtime = time_code([&]() { completion = MST(cluster_count, unmapped_completion_edges); });
    return std::make_tuple(completion, runtime);
};

std::vector<WeightedEdge> map_completion_edges(std::vector<CompletionEdge> completion) {
    return std::ranges::to<std::vector>(completion | std::views::transform([&](CompletionEdge e) {
                                            return WeightedEdge{
                                                .weight = e.weight,
                                                .a = e.a_rep,
                                                .b = e.b_rep,
                                            };
                                        }));
};