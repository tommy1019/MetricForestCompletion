#pragma once

#include <string>
#include <tuple>

#include "algo/k_centering.h"
#include "lib/test_runner.h"

// Generates a clustering evaluator for a given amount of clusters
template <typename Vec, typename DistFunc>
std::pair<std::string, EvaluatorType<Vec, size_t>> fixed_cluster(size_t cluster_count, DistFunc orig_dist_func) {
    return {"C" + std::to_string(cluster_count), [cluster_count, orig_dist_func](std::vector<Vec> points, size_t N) -> EvaluatorReturnType {
                // Code to count dist calls
                size_t dist_calls = 0;
                auto counting_dist_func = [&](const Vec& a, const Vec& b) {
                    dist_calls++;
                    return orig_dist_func(a, b);
                };
                auto get_dist_calls = [&]() {
                    size_t res = dist_calls;
                    dist_calls = 0;
                    return res;
                };

                // Run k-centering, and find the inital forest. This is shared between all code-paths below
                auto clustering = k_centering(points, cluster_count, counting_dist_func);
                size_t clustering_dist_calls = get_dist_calls();

                auto cluster_vecs = create_cluster_vecs(cluster_count, points, clustering.assignments);
                auto [cluster_msts, sub_cluster_runtime] = sub_clusters(cluster_count, points, cluster_vecs, counting_dist_func);
                size_t sub_cluster_dist_calls = get_dist_calls();

                auto f = [&](auto F) -> MetricForestCompletion {
                    auto [unmapped_completion_edges, completion_edges_runtime] = F(cluster_count, points, cluster_vecs, counting_dist_func);

                    auto [completion, completion_runtime] = get_completion(cluster_count, unmapped_completion_edges);
                    auto completion_edges = map_completion_edges(completion);

                    size_t mfc_dist_calls = get_dist_calls();

                    MetricForestCompletion mfc{
                        .cluster_edges = cluster_msts,
                        .completion_edges = completion_edges,
                        .sub_cluster_runtime = sub_cluster_runtime,
                        .completion_edges_runtime = completion_edges_runtime,
                        .completion_runtime = completion_runtime,

                        .clustering_dist_calls = clustering_dist_calls,
                        .sub_cluster_dist_calls = sub_cluster_dist_calls,
                        .mfc_dist_calls = mfc_dist_calls,
                    };

                    return mfc;
                };

                // Simple is algorithm from original paper as a baseline
                co_yield std::make_pair("simple", std::tuple{clustering, f([]<typename... Args>(Args... args) { return get_unmapped_completion_edges_approx_simple(args...); })});
                // Plus edge checks one additional edge as a potential huristic
                co_yield std::make_pair("plus_edge", std::tuple{clustering, f([]<typename... Args>(Args... args) { return get_unmapped_completion_edges_approx_simple_plus_edge(args...); })});
                // Opt optimally sovles the MFC problem
                co_yield std::make_pair("opt", std::tuple{clustering, f([]<typename... Args>(Args... args) { return get_unmapped_completion_edges_opt(args...); })});

                // Fixed reps per comp
                for (size_t reps_per_comp = 1; reps_per_comp <= 41; reps_per_comp += 2) {

                    float reps_cost = 0;

                    // Find 'reps_per_comp' reps for each component
                    auto [find_reps_runtime, reps] = time_code_ret([&]() {
                        std::vector<std::vector<std::pair<size_t, float>>> res;
                        for (auto& v : cluster_vecs) {
                            res.push_back(get_best_reps(cluster_count, points, v, reps_per_comp, counting_dist_func));
                        }
                        for (auto v : res) {
                            reps_cost += v.back().second;
                        }
                        return res;
                    });

                    // Use the reps to find potential edges connecting components
                    auto [unmapped_completion_edges, completion_edges_runtime] = get_unmapped_completion_edges_from_reps(points, cluster_vecs, reps, counting_dist_func);

                    // Run MST on the potential edges
                    auto [completion, completion_runtime] = get_completion(cluster_count, unmapped_completion_edges);
                    // Map edges back to global ids
                    auto completion_edges = map_completion_edges(completion);

                    size_t mfc_dist_calls = get_dist_calls();

                    MetricForestCompletion mfc{
                        .cluster_edges = cluster_msts,
                        .completion_edges = completion_edges,

                        .rep_count = reps_per_comp * cluster_count,

                        .sub_cluster_runtime = sub_cluster_runtime,
                        .completion_edges_runtime = completion_edges_runtime,
                        .completion_runtime = completion_runtime,
                        .find_reps_runtime = find_reps_runtime,

                        .reps_cost = reps_cost,

                        .clustering_dist_calls = clustering_dist_calls,
                        .sub_cluster_dist_calls = sub_cluster_dist_calls,
                        .mfc_dist_calls = mfc_dist_calls,
                    };

                    // Yeild the results back to the test runner
                    co_yield std::make_pair("fixed_reps_" + std::to_string(reps_per_comp), std::tuple{clustering, mfc});
                }

                for (float budget_mult = 1.0f; budget_mult <= 40.0; budget_mult += 2) {

                    // Must subtract cluster_count to account for the required rep per component
                    size_t budget = cluster_count * budget_mult - cluster_count;

                    if (budget > N)
                        budget = N;

                    // Find b reps per component, reused for both greedy and DP
                    auto [find_reps_runtime, all_reps] = time_code_ret([&]() {
                        std::vector<std::pair<size_t, std::vector<std::pair<size_t, float>>>> res;
                        for (auto& v : cluster_vecs) {
                            res.push_back({0, get_best_reps(cluster_count, points, v, budget + 1, counting_dist_func)}); // Plus 1 for the required one per component
                        }
                        return res;
                    });
                    size_t find_reps_dist_calls = get_dist_calls();

                    // Greedy
                    {
                        std::vector<std::vector<std::pair<size_t, float>>> final_reps;

                        float final_cost = 0;

                        // Run greedy method to pick reps
                        auto pick_reps_runtime = time_code([&]() {
                            // Must include first rep from each comp
                            for (auto& c : all_reps) {
                                final_reps.emplace_back();
                                final_reps.back().push_back(*c.second.begin());
                                c.first++;
                            }

                            size_t reps_used = 0;
                            while (reps_used < budget) {
                                // Find rep that has the maximum decrease in the min distance of a component
                                float max = -1;
                                size_t max_cluster_index;
                                bool found = false;
                                for (size_t cluster_index = 0; cluster_index < cluster_count; cluster_index++) {
                                    size_t cur_clust_ele = all_reps[cluster_index].first;

                                    // Check that we still have reps to pick in this component
                                    if (cur_clust_ele >= all_reps[cluster_index].second.size())
                                        continue;

                                    // Cost change by including another rep from this component
                                    float diff = all_reps[cluster_index].second[cur_clust_ele - 1].second - all_reps[cluster_index].second[cur_clust_ele].second;

                                    if (diff > max) {
                                        max = diff;
                                        max_cluster_index = cluster_index;
                                        found = true;
                                    }
                                }

                                // Sanity check for running out of reps
                                if (!found) {
                                    std::print("Ran out of reps to pick from, something went wrong reps: {}, budget: {}\n", reps_used, budget);
                                    break;
                                }

                                final_reps[max_cluster_index].push_back(all_reps[max_cluster_index].second[all_reps[max_cluster_index].first]);
                                all_reps[max_cluster_index].first++;

                                reps_used++;
                            }

                            // Get final_cost for calculating alpha later
                            for (auto& v : all_reps) {
                                final_cost += v.second[v.first - 1].second;
                            }
                        });

                        // Recount the number of reps picked to double check
                        size_t rep_count_double_check = 0;
                        for (auto& t : final_reps)
                            rep_count_double_check += t.size();

                        // Use the reps to find potential edges connecting components
                        auto [unmapped_completion_edges, completion_edges_runtime] = get_unmapped_completion_edges_from_reps(points, cluster_vecs, final_reps, counting_dist_func);

                        // Run MST on the potential edges
                        auto [completion, completion_runtime] = get_completion(cluster_count, unmapped_completion_edges);
                        // Map edges back to global ids
                        auto completion_edges = map_completion_edges(completion);

                        size_t mfc_dist_calls = find_reps_dist_calls + get_dist_calls();

                        MetricForestCompletion mfc{
                            .cluster_edges = cluster_msts,
                            .completion_edges = completion_edges,

                            .rep_count = rep_count_double_check,

                            .sub_cluster_runtime = sub_cluster_runtime,
                            .completion_edges_runtime = completion_edges_runtime,
                            .completion_runtime = completion_runtime,
                            .find_reps_runtime = find_reps_runtime,
                            .pick_reps_runtime = pick_reps_runtime,

                            .reps_cost = final_cost,

                            .clustering_dist_calls = clustering_dist_calls,
                            .sub_cluster_dist_calls = sub_cluster_dist_calls,
                            .mfc_dist_calls = mfc_dist_calls,
                        };

                        // Yeild the results back to the test runner
                        co_yield std::make_pair("greedy_" + std::to_string(budget_mult), std::tuple{clustering, mfc});
                    }

                    // DP
                    {
                        std::vector<std::vector<std::pair<size_t, float>>> final_reps;

                        float final_cost = 0;

                        // Calculate fallback final cost for b=0
                        for (auto v : all_reps) {
                            final_cost += v.second.begin()->second;
                        }

                        // Run DP method to pick reps
                        auto pick_reps_runtime = time_code([&]() {
                            // Must include first rep from each comp
                            for (auto& c : all_reps) {
                                final_reps.emplace_back();
                                final_reps.back().push_back(*c.second.begin());
                            }

                            ssize_t T = cluster_count;
                            ssize_t B = budget;

                            if (B <= 0)
                                return;

                            std::vector<float> costs;
                            costs.resize(B + 1, 0.0f); // B + 1 includes budget of zero

                            std::vector<size_t> work;
                            work.resize((B + 1) * T, 0); // B + 1 includes budget of zero

                            auto idx = [&](ssize_t b, ssize_t t) { return b + t * (B + 1); };

                            for (size_t b = 0; b <= B; b++) {
                                costs[b] = all_reps[0].second[b].second; // Get costs for first row
                                work[idx(b, 0)] = b;
                            }

                            for (size_t t = 1; t < T; t++) {
                                for (ssize_t b = B; b >= 0; b--) {
                                    // Compare taking i reps from the new component and the budget b - i from the previous row
                                    float min = INFINITY;
                                    ssize_t min_index = 0;

                                    for (ssize_t i = 0; i <= b; i++) {
                                        float cur_cost = costs[i] + all_reps[t].second[b - i].second;
                                        if (cur_cost < min) {
                                            min_index = i;
                                            min = cur_cost;
                                        }
                                    }

                                    // Select the best option and copy the element from the previous row
                                    costs[b] = min;

                                    size_t sum = 0;

                                    work[idx(b, t)] = b - min_index;
                                    sum += b - min_index;

                                    for (int j = 0; j < t; j++) {
                                        work[idx(b, j)] = work[idx(min_index, j)];
                                        sum += work[idx(b, j)];
                                    }

                                    // Sanity check that the sum of each entry in the matrix is b
                                    if (sum != b) {
                                        std::print("t: {:3}      {:6} != {:6}\n", t, sum, b);
                                    }
                                }
                            }

                            // Grab the final_cost and number of reps from each component from the last matrix entry
                            final_cost = costs[B];

                            for (size_t t = 0; t < T; t++) {
                                for (size_t i = 1; i <= work[idx(B, t)]; i++) {
                                    final_reps[t].push_back(all_reps[t].second[i]);
                                }
                            }
                        });

                        // Recount the number of reps picked to double check
                        size_t rep_count_double_check = 0;
                        for (auto& t : final_reps)
                            rep_count_double_check += t.size();

                        // Use the reps to find potential edges connecting components
                        auto [unmapped_completion_edges, completion_edges_runtime] = get_unmapped_completion_edges_from_reps(points, cluster_vecs, final_reps, counting_dist_func);

                        // Run MST on the potential edges
                        auto [completion, completion_runtime] = get_completion(cluster_count, unmapped_completion_edges);
                        // Map edges back to global ids
                        auto completion_edges = map_completion_edges(completion);

                        size_t mfc_dist_calls = find_reps_dist_calls + get_dist_calls();

                        MetricForestCompletion mfc{
                            .cluster_edges = cluster_msts,
                            .completion_edges = completion_edges,

                            .rep_count = rep_count_double_check,

                            .sub_cluster_runtime = sub_cluster_runtime,
                            .completion_edges_runtime = completion_edges_runtime,
                            .completion_runtime = completion_runtime,
                            .find_reps_runtime = find_reps_runtime,
                            .pick_reps_runtime = pick_reps_runtime,

                            .reps_cost = final_cost,

                            .clustering_dist_calls = clustering_dist_calls,
                            .sub_cluster_dist_calls = sub_cluster_dist_calls,
                            .mfc_dist_calls = mfc_dist_calls,
                        };

                        // Yeild the results back to the test runner
                        co_yield std::make_pair("dp_" + std::to_string(budget_mult), std::tuple{clustering, mfc});
                    }
                }
            }};
};

// Runs the standard set of evalulators for N=30000
template <typename Vec, typename GenFunc, typename DistFunc>
void run_standard_evalulators(
    std::string output_file, std::string all_output_file, bool cluster_detection_test, GenFunc&& gen_func, DistFunc&& dist_func, std::optional<size_t> N_Override = std::nullopt) {

    size_t N = 30000;
    if (N_Override.has_value()) // N override is for Jaccard cooking
        N = *N_Override;

    size_t sqrtN = std::floor(std::sqrt(N));

    // List of evaluators to run
    std::vector<std::pair<std::string, EvaluatorType<Vec, size_t>>> evaluators = {{
        fixed_cluster<Vec>(sqrtN, dist_func),
        fixed_cluster<Vec>(sqrtN / 2, dist_func),
        fixed_cluster<Vec>(sqrtN / 4, dist_func),
    }};

    if (cluster_detection_test) {
        // Replace the set evaluators with a list of every cluster amount from 2 to 150
        evaluators.clear();
        for (int i = 2; i < 150; i++) {
            evaluators.push_back(fixed_cluster<Vec>(i, dist_func));
        }

        // Create and run a test runner
        auto test_runner = MUST(CreateTestRunner<Vec, true, size_t>(output_file, all_output_file, std::array<std::string, 1>{"N"}, dist_func, gen_func, evaluators));
        MUST(test_runner.run_test(32, N));
    } else {
        // Create and run a test runner
        auto test_runner = MUST(CreateTestRunner<Vec, true, size_t>(output_file, all_output_file, std::array<std::string, 1>{"N"}, dist_func, gen_func, evaluators));
        MUST(test_runner.run_test(16, N));
    }
}