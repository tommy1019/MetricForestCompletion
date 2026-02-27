#pragma once

#include "metric_forest_completion_functions.h"

// Runs the MFC algorithm. Takes a list of points and a clustering as input, runs optimal MST on the clusters and completes the MST approximation for the entire list of points

template <typename T, typename DistFunc>
MetricForestCompletion metric_forest_completion(std::vector<T> points, size_t cluster_count, std::vector<size_t> cluster_assignments, DistFunc dist_func) {

    auto cluster_vecs = create_cluster_vecs(cluster_count, points, cluster_assignments);
    auto [cluster_msts, sub_cluster_runtime] = sub_clusters(cluster_count, points, cluster_vecs, dist_func);
    auto [unmapped_completion_edges, completion_edges_runtime] = get_unmapped_completion_edges_approx_simple(cluster_count, points, cluster_vecs, dist_func);
    auto [completion, completion_runtime] = get_completion(cluster_count, unmapped_completion_edges);
    auto completion_edges = map_completion_edges(completion);

    MetricForestCompletion mfc{
        .cluster_edges = cluster_msts,
        .completion_edges = completion_edges,

        .sub_cluster_runtime = sub_cluster_runtime,
        .completion_edges_runtime = completion_edges_runtime,
        .completion_runtime = completion_runtime,
    };

    return mfc;
}