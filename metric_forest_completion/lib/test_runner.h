#pragma once

#include <fstream>
#include <functional>
#include <future>
#include <map>
#include <ostream>
#include <random>
#include <set>

#include "../algo/clustering.h"
#include "../algo/metric_forest_completion.h"

#include "error.h"
#include "generator.h"

#define DATA_POINT_LIST                                                                                                                                                                                \
    L(MFC_Cost, mfc_cost, mfc_cost)                                                                                                                                                                    \
    L(MFC_Completion_Cost, completion_cost, completion_cost)                                                                                                                                           \
    L(MFC_Runtime, mfc_runtime, clustering.runtime + mfc.sub_cluster_runtime + mfc.find_reps_runtime + mfc.pick_reps_runtime + mfc.completion_edges_runtime + mfc.completion_runtime)                  \
    L(Gamma, gamma, gamma)                                                                                                                                                                             \
    L(Cluster_Size_Mu, cluster_size_mu, cluster_size_mu)                                                                                                                                               \
    L(Cluster_Size_Sigma, cluster_size_sigma, cluster_size_sigma)                                                                                                                                      \
    L(Sub_Clustering_Runtime, sub_cluster_runtime, mfc.sub_cluster_runtime)                                                                                                                            \
    L(Completion_Edges_Runtime, completion_edges_runtime, mfc.completion_edges_runtime)                                                                                                                \
    L(Completion_Runtime, completion_runtime, mfc.completion_runtime)                                                                                                                                  \
    L(Clustering_Runtime, clustering_runtime, clustering.runtime)                                                                                                                                      \
    L(Find_Reps_Runtime, find_reps_runtime, mfc.find_reps_runtime)                                                                                                                                     \
    L(Pick_Reps_Runtime, pick_reps_runtime, mfc.pick_reps_runtime)                                                                                                                                     \
    L(Rep_Count, rep_count, (double)mfc.rep_count)                                                                                                                                                     \
    L(Reps_Cost, reps_cost, mfc.reps_cost)                                                                                                                                                             \
    L(Clustering_Dist_Calls, clustering_dist_calls, (double)mfc.clustering_dist_calls)                                                                                                                 \
    L(Sub_Clustering_Dist_Calls, sub_cluster_dist_calls, (double)mfc.sub_cluster_dist_calls)                                                                                                           \
    L(MFC_Dist_Calls, mfc_dist_calls, (double)mfc.mfc_dist_calls)                                                                                                                                      \
    L(Dist_Calls, dist_calls, (double)(mfc.clustering_dist_calls + mfc.sub_cluster_dist_calls + mfc.mfc_dist_calls))

// TestHarness, see .cpp files for use

using EvaluatorReturnType = std::generator<std::pair<std::string, std::tuple<Clustering, MetricForestCompletion>>>;

template <typename Point, typename... Args>
using EvaluatorType = std::function<EvaluatorReturnType(std::vector<Point>, Args...)>;

template <bool MultiThread, typename Point, typename FDistFunc, typename FDatasetGenerator, typename... Args>
struct TestRunner {
  public:
    using Evaluator = EvaluatorType<Point, Args...>;

    TestRunner(std::ofstream& out,
               std::ofstream& all_out,
               std::default_random_engine random_engine,
               std::array<std::string, sizeof...(Args)> args_headers,
               FDistFunc dist_func,
               FDatasetGenerator dataset_generator,
               std::vector<std::pair<std::string, Evaluator>> evaluators)
        : m_out(std::move(out)), m_all_out(std::move(all_out)), m_random_engine(random_engine), m_args_headers(args_headers), m_dist_func(dist_func), m_dataset_generator(dataset_generator),
          m_evaluators(std::move(evaluators)) {}

    TestRunner(const TestRunner&) = delete;
    TestRunner& operator=(const TestRunner&) = delete;
    TestRunner(TestRunner&&) = default;
    TestRunner& operator=(TestRunner&&) = default;

    ErrorOr<void> write_headers() {

        std::print(m_out, "N_mu, N_sigma");

        for (auto& s : m_args_headers)
            std::print(m_out, ", {}", s);

        std::print(m_out,
                   ", MST_Cost_mu, MST_Cost_sigma"
                   ", MST_Runtime_mu, MST_Runtime_sigma, RunType");
        for (auto e : m_evaluators) {

#define L(NAME, EVAL_VAR, INPUT_VAR) std::print(m_out, ", {}_" #NAME "_mu, {}_" #NAME "_sigma", e.first, e.first);
            DATA_POINT_LIST
#undef L
        }
        std::print(m_out, "\n");
        m_out.flush();

        std::print(m_all_out, "N");

        for (auto& s : m_args_headers)
            std::print(m_all_out, ", {}", s);

        std::print(m_all_out, ", MST_Cost, MST_Runtime, RunType");

        for (auto e : m_evaluators) {
#define L(NAME, EVAL_VAR, INPUT_VAR) std::print(m_all_out, ", {}_" #NAME, e.first);
            DATA_POINT_LIST
#undef L
        }
        std::print(m_all_out, "\n");
        m_all_out.flush();

        return {};
    }

    ErrorOr<void> run_test(size_t repeats, const Args&... args) {

        struct EvaulatorResults {

#define L(NAME, EVAL_VAR, INPUT_VAR) double EVAL_VAR;
            DATA_POINT_LIST
#undef L
        };

        struct Results {
            size_t n;
            double mst_cost;
            double mst_runtime;

            std::map<std::string, std::vector<EvaulatorResults>> evaluator_res;
        };

        auto execute_test = [&](std::vector<Point> points) -> Results {
            auto [mst, cur_mst_runtime] = time_code([&]() { return MST_Implicit(points, m_dist_func); });

            double cur_mst_cost = 0;
            for (auto& e : mst)
                cur_mst_cost += e.weight;

            std::map<std::string, std::vector<EvaulatorResults>> evaluator_results;
            // evaluator_results.reserve(m_evaluators.size());

            for (size_t j = 0; j < m_evaluators.size(); j++) {

                for (auto [key_name, evalulator_res] : m_evaluators[j].second(points, args...)) {
                    auto [clustering, mfc] = std::move(evalulator_res);

                    double mfc_cluster_weights = 0;
                    for (auto c : mfc.cluster_edges)
                        for (auto e : c)
                            mfc_cluster_weights += e.weight;

                    double completion_cost = 0;
                    for (auto e : mfc.completion_edges)
                        completion_cost += e.weight;

                    double mfc_cost = mfc_cluster_weights + completion_cost;

                    auto gamma = [&]() {
                        double bot = 0;
                        for (auto e : mst) {
                            if (clustering.assignments[e.a] == clustering.assignments[e.b]) {
                                bot += e.weight;
                            }
                        }
                        return mfc_cluster_weights / bot;
                    }();

                    std::vector<double> cluster_sizes(mfc.cluster_edges.size(), 0.0);
                    for (auto v : clustering.assignments)
                        cluster_sizes[v]++;
                    auto [cluster_size_mu, cluster_size_sigma] = compute_stats(cluster_sizes);

                    evaluator_results[key_name].push_back(EvaulatorResults{
#define L(NAME, EVAL_VAR, INPUT_VAR) .EVAL_VAR = INPUT_VAR,
                        DATA_POINT_LIST
#undef L
                    });
                }
            }

            return Results{.n = points.size(), .mst_cost = cur_mst_cost, .mst_runtime = cur_mst_runtime, .evaluator_res = evaluator_results};
        };

        auto write_individual_results = [&](std::set<std::string> keys, Results res) {
            for (auto& key : keys) {
                std::print(m_all_out, "{}", res.n);
                (std::print(m_all_out, ", {}", args), ...);
                std::print(m_all_out, ", {}, {}", res.mst_cost, res.mst_runtime);
                std::print(m_all_out, ", {}", key);

                for (size_t j = 0; j < m_evaluators.size(); j++) {
                    EvaulatorResults& cur = res.evaluator_res[key][j];
                    std::print(m_all_out,
#define L(NAME, EVAL_VAR, INPUT_VAR) ", {}"
                               DATA_POINT_LIST
#undef L

#define L(NAME, EVAL_VAR, INPUT_VAR) , cur.EVAL_VAR
                                   DATA_POINT_LIST
#undef L
                    );
                }

                std::print(m_all_out, "\n");
                m_all_out.flush();
            }
        };

        std::vector<Results> results;

        if constexpr (MultiThread) {
            std::vector<std::future<Results>> futures;
            for (size_t i = 0; i < repeats; i++) {
                auto points = TRY(m_dataset_generator(m_random_engine, args...));
                futures.push_back(std::async(std::launch::async, execute_test, points));
            }
            for (size_t i = 0; i < repeats; i++) {
                results.push_back(futures[i].get());
            }
        } else {
            for (size_t i = 0; i < repeats; i++) {
                auto points = TRY(m_dataset_generator(m_random_engine, args...));
                results.push_back(execute_test(points));
            }
        }

        std::set<std::string> keys;
        for (auto& r : results) {
            for (auto [key, value] : r.evaluator_res)
                keys.insert(key);
        }

        for (auto& res : results)
            write_individual_results(keys, res);

        auto extract = [](auto vec, auto f) {
            std::vector<decltype(f(vec[0]))> res;
            for (auto& v : vec)
                res.push_back(f(v));
            return res;
        };

        for (auto& key : keys) {
            auto [n_mu, n_sigma] = compute_stats(extract(results, [](Results& r) { return (double)r.n; }));
            auto [mst_cost_mu, mst_cost_sigma] = compute_stats(extract(results, [](Results& r) { return r.mst_cost; }));
            auto [mst_time_mu, mst_time_sigma] = compute_stats(extract(results, [](Results& r) { return r.mst_runtime; }));

            std::print(m_out, "{}, {}", n_mu, n_sigma);
            (std::print(m_out, ", {}", args), ...);
            std::print(m_out, ", {}, {}, {}, {}", mst_cost_mu, mst_cost_sigma, mst_time_mu, mst_time_sigma);
            std::print(m_out, ", {}", key);

            for (size_t j = 0; j < m_evaluators.size(); j++) {

                auto p = [&](std::tuple<double, double> v) { std::print(m_out, ", {}, {}", std::get<0>(v), std::get<1>(v)); };

#define L(NAME, EVAL_VAR, INPUT_VAR) p(compute_stats(extract(results, [&](Results& r) { return r.evaluator_res[key][j].EVAL_VAR; })));
                DATA_POINT_LIST
#undef L
            }
            std::print(m_out, "\n");
            m_out.flush();
        }

        return {};
    }

  private:
    std::ofstream m_out;
    std::ofstream m_all_out;

    std::array<std::string, sizeof...(Args)> m_args_headers;

    FDistFunc m_dist_func;
    FDatasetGenerator m_dataset_generator;
    std::vector<std::pair<std::string, Evaluator>> m_evaluators;

    std::default_random_engine m_random_engine;

    constexpr static auto time_code(auto f) {
        auto start = std::chrono::high_resolution_clock::now();
        auto res = f();
        auto end = std::chrono::high_resolution_clock::now();
        return std::make_pair(res, std::chrono::duration<double, std::milli>(end - start).count());
    }

    constexpr static std::tuple<double, double> compute_stats(const std::vector<double>& vals) {
        double avg = 0;
        for (auto v : vals)
            avg += v;
        avg /= vals.size();
        double stddev = 0;
        for (auto v : vals)
            stddev += std::pow(v - avg, 2);
        stddev = std::sqrt(stddev / vals.size());
        return {avg, stddev};
    }
};

/**
    Creates a test runner.
    Template Types:
    Point - type of a single data point
    MultiThread - true/false for multithreaded test running
    Args... - A list of extra arguments provided to the run_test function, these arguments are written to the out files as well as copied to the generate and cluster functions
    FDistFunc - Type of the distance function used
    FDatasetGenerator - Type of the dataset generator function used
    Arguments:
    results_file - file name for file to write average test results to
    all_tests_file - file name for file to write individual tests to
    args_headers - Header names for extra arguments provided to the test function
    dist_func - distance function for points, take two points as arguments and returns a floating point for their distance. Called in parallel when multithreading is enabled
    dataset_generator - function to generate a new dataset. Is passed a std::default_random_engine& as well as any arguments specified in Args.... Never called in parallel
    evaluators - a list std::pair<std::string, std::function>. The string is the header prefix to use in the output files, where the function takes a list of points and the Args... and returns a
                    std::generator<std::pair<std::string, std::tuple<Clustering, MetricForestCompletion>>>. Each yeild of this function generates a single line in the output files labled with the key
                    given as the first in the pair. Called in parallel when multithreading is enabled
 */
template <typename Point, bool MultiThread = false, typename... Args, typename FDistFunc, typename FDatasetGenerator>
static ErrorOr<TestRunner<MultiThread, Point, FDistFunc, FDatasetGenerator, Args...>>
CreateTestRunner(std::string results_file,
                 std::string all_tests_file,
                 std::array<std::string, sizeof...(Args)> args_headers,
                 FDistFunc dist_func,
                 FDatasetGenerator dataset_generator,
                 std::vector<std::pair<std::string, typename TestRunner<MultiThread, Point, FDistFunc, FDatasetGenerator, Args...>::Evaluator>> evaluators) {
    auto out = std::ofstream(results_file);
    if (!out)
        return ERR("Failed to open file '" + results_file + "' for writing");

    auto all_out = std::ofstream(all_tests_file);
    if (!all_out)
        return ERR("Failed to open file '" + all_tests_file + "' for writing");

    TestRunner<MultiThread, Point, FDistFunc, FDatasetGenerator, Args...> res(out, all_out, std::default_random_engine(std::random_device{}()), args_headers, dist_func, dataset_generator, evaluators);
    TRY(res.write_headers());
    return res;
}