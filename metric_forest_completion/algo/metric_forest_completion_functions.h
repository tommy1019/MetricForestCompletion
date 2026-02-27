#pragma once

#include <cfloat>
#include <cmath>

#include "metric_forest_completion_utils.h"

template <typename T, typename F>
std::tuple<std::vector<CompletionEdge>, double>
get_unmapped_completion_edges_from_reps(std::vector<T>& points, std::vector<std::vector<size_t>>& cluster_vecs, std::vector<std::vector<std::pair<size_t, float>>>& rep_vecs, F& dist_func) {
    size_t cluster_count = cluster_vecs.size();

    std::vector<CompletionEdge> unmapped_completion_edges;
    unmapped_completion_edges.reserve(cluster_count * (cluster_count - 1));

    auto runtime = time_code([&]() {
        for (size_t i = 0; i < cluster_count - 1; i++) {
            for (size_t j = 1; j < cluster_count; j++) {

                if (cluster_vecs[i].size() == 0 || cluster_vecs[j].size() == 0)
                    continue;

                CompletionEdge e;
                e.a = i;
                e.b = j;
                e.weight = INFINITY;

                for (size_t k = 0; k < cluster_vecs[j].size(); k++) {
                    for (auto [i_rep, cost] : rep_vecs[i]) {
                        auto dist = dist_func(points[i_rep], points[cluster_vecs[j][k]]);
                        if (dist < e.weight) {
                            e.a_rep = i_rep;
                            e.b_rep = cluster_vecs[j][k];
                            e.weight = dist;
                        }
                    }
                }

                for (size_t k = 0; k < cluster_vecs[i].size(); k++) {
                    for (auto [j_rep, cost] : rep_vecs[j]) {
                        auto dist = dist_func(points[cluster_vecs[i][k]], points[j_rep]);
                        if (dist < e.weight) {
                            e.a_rep = cluster_vecs[i][k];
                            e.b_rep = j_rep;
                            e.weight = dist;
                        }
                    }
                }

                unmapped_completion_edges.push_back(e);
            }
        }
    });

    return std::make_tuple(unmapped_completion_edges, runtime);
};

template <typename T, typename F>
std::vector<std::pair<size_t, float>> get_best_reps(size_t cluster_count, std::vector<T>& points, std::vector<size_t>& cluster, size_t amount, F& dist_func) {
    std::vector<std::pair<size_t, float>> reps;

    if (amount <= 0)
        return reps;

    reps.reserve(amount);
    reps.emplace_back(cluster[0], INFINITY);

    std::vector<float> cur_distances;
    cur_distances.reserve(points.size());

    {
        size_t second_index = cluster[0];
        auto max_dist = dist_func(points[cluster[0]], points[reps.back().first]);

        for (size_t i = 0; i < cluster.size(); i++) {
            auto dist = dist_func(points[cluster[i]], points[reps.back().first]);
            if (dist > max_dist) {
                second_index = cluster[i];
                max_dist = dist;
            }
            cur_distances.push_back(dist);
        }

        reps.back().second = max_dist;

        if (amount <= 1)
            return reps;

        reps.emplace_back(second_index, INFINITY);
    }

    while (reps.size() <= amount) {
        size_t new_index = cluster[0];
        auto max_dist = std::min(dist_func(points[cluster[0]], points[reps.back().first]), cur_distances[0]);
        cur_distances[0] = max_dist;

        for (size_t i = 1; i < cluster.size(); i++) {
            auto dist = dist_func(points[cluster[i]], points[reps.back().first]);
            if (dist < cur_distances[i])
                cur_distances[i] = dist;

            if (cur_distances[i] > max_dist) {
                max_dist = cur_distances[i];
                new_index = cluster[i];
            }
        }

        reps.back().second = max_dist;

        if (reps.size() == amount)
            return reps;

        reps.emplace_back(new_index, INFINITY);
    }

    return reps;
}

template <typename T, typename F>
std::tuple<std::vector<CompletionEdge>, double>
get_unmapped_completion_edges_approx_simple(size_t cluster_count, std::vector<T>& points, std::vector<std::vector<size_t>>& cluster_vecs, F& dist_func) {
    std::vector<CompletionEdge> unmapped_completion_edges;
    unmapped_completion_edges.reserve(cluster_count * (cluster_count - 1));

    auto runtime = time_code([&]() {
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

    return std::make_tuple(unmapped_completion_edges, runtime);
};

template <typename T, typename F>
std::tuple<std::vector<CompletionEdge>, double>
get_unmapped_completion_edges_approx_simple_plus_edge(size_t cluster_count, std::vector<T>& points, std::vector<std::vector<size_t>>& cluster_vecs, F& dist_func) {
    std::vector<CompletionEdge> unmapped_completion_edges;
    unmapped_completion_edges.reserve(cluster_count * (cluster_count - 1));

    auto runtime = time_code([&]() {
        for (size_t clust_i = 0; clust_i < cluster_count - 1; clust_i++) {
            for (size_t clust_j = 1; clust_j < cluster_count; clust_j++) {

                if (cluster_vecs[clust_i].size() == 0 || cluster_vecs[clust_j].size() == 0)
                    continue;

                size_t clust_i_rep = 0;
                size_t clust_j_rep = 0;

                CompletionEdge e1;
                e1.a = clust_i;
                e1.b = clust_j;
                e1.weight = INFINITY;

                for (size_t k = 0; k < cluster_vecs[clust_j].size(); k++) {
                    auto dist = dist_func(points[cluster_vecs[clust_i][clust_i_rep]], points[cluster_vecs[clust_j][k]]);
                    if (dist < e1.weight) {
                        e1.a_rep = cluster_vecs[clust_i][clust_i_rep];
                        e1.b_rep = cluster_vecs[clust_j][k];
                        e1.weight = dist;
                    }
                }

                CompletionEdge e2;
                e2.a = clust_i;
                e2.b = clust_j;
                e2.weight = INFINITY;

                for (size_t k = 0; k < cluster_vecs[clust_i].size(); k++) {
                    auto dist = dist_func(points[cluster_vecs[clust_i][k]], points[cluster_vecs[clust_j][clust_j_rep]]);
                    if (dist < e2.weight) {
                        e2.a_rep = cluster_vecs[clust_i][k];
                        e2.b_rep = cluster_vecs[clust_j][clust_j_rep];
                        e2.weight = dist;
                    }
                }

                CompletionEdge e;
                if (e1.weight <= e2.weight)
                    e = e1;
                else
                    e = e2;

                if (float dist = dist_func(points[e2.a_rep], points[e1.b_rep]); e.weight > dist) {
                    e = {
                        .a = clust_i,
                        .b = clust_j,
                        .a_rep = e2.a_rep,
                        .b_rep = e1.b_rep,
                        .weight = dist,
                    };
                }

                unmapped_completion_edges.push_back(e);
            }
        }
    });

    return std::make_tuple(unmapped_completion_edges, runtime);
};

template <typename T, typename F>
std::tuple<std::vector<CompletionEdge>, double> get_unmapped_completion_edges_opt(size_t cluster_count, std::vector<T>& points, std::vector<std::vector<size_t>>& cluster_vecs, F& dist_func) {
    std::vector<CompletionEdge> unmapped_completion_edges;
    unmapped_completion_edges.reserve(cluster_count * (cluster_count - 1));

    auto runtime = time_code([&]() {
        for (size_t clust_i = 0; clust_i < cluster_count - 1; clust_i++) {
            for (size_t clust_j = 1; clust_j < cluster_count; clust_j++) {

                if (cluster_vecs[clust_i].size() == 0 || cluster_vecs[clust_j].size() == 0)
                    continue;

                CompletionEdge e;
                e.a = clust_i;
                e.b = clust_j;
                e.weight = INFINITY;

                for (size_t i = 0; i < cluster_vecs[clust_i].size(); i++) {
                    for (size_t j = 0; j < cluster_vecs[clust_j].size(); j++) {
                        auto dist = dist_func(points[cluster_vecs[clust_i][i]], points[cluster_vecs[clust_j][j]]);
                        if (dist < e.weight) {
                            e.a_rep = cluster_vecs[clust_i][i];
                            e.b_rep = cluster_vecs[clust_j][j];
                            e.weight = dist;
                        }
                    }
                }

                unmapped_completion_edges.push_back(e);
            }
        }
    });

    return std::make_tuple(unmapped_completion_edges, runtime);
};
