#pragma once

#include <fstream>
#include <functional>
#include <future>
#include <random>

#include "../algo/clustering.h"
#include "../algo/metric_forest_completion.h"

#include "error.h"

// TestHarness, see .cpp files for use

template <bool MultiThread, typename Point, typename FDistFunc, typename FDatasetGenerator, typename... Args>
struct TestRunner {
  public:
    using Evaluator = std::function<std::tuple<Clustering, MetricForestCompletion>(std::vector<Point>, Args...)>;

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
                   ", MST_Runtime_mu, MST_Runtime_sigma");
        for (auto e : m_evaluators) {
            std::print(m_out, ", {}_MFC_Cost_mu, {}_MFC_Cost_sigma", e.first, e.first);
            std::print(m_out, ", {}_MFC_Runtime_mu, {}_MFC_Runtime_sigma", e.first, e.first);
            std::print(m_out, ", {}_Gamma_mu, {}_Gamma_sigma", e.first, e.first);
            std::print(m_out, ", {}_Cluster_Size_Mu_mu, {}_Cluster_Size_Mu_sigma", e.first, e.first);
            std::print(m_out, ", {}_Cluster_Size_Sigma_mu, {}_Cluster_Size_Sigma_sigma", e.first, e.first);
            std::print(m_out, ", {}_Sub_Clustering_Runtime_mu, {}_Sub_Clustering_Runtime_sigma", e.first, e.first);
            std::print(m_out, ", {}_Completion_Edges_Runtime_mu, {}_Completion_Edges_Runtime_sigma", e.first, e.first);
            std::print(m_out, ", {}_Completion_Runtime_mu, {}_Completion_Runtime_sigma", e.first, e.first);
            std::print(m_out, ", {}_Clustering_Runtime_mu, {}_Clustering_Runtime_sigma", e.first, e.first);
        }
        std::print(m_out, "\n");
        m_out.flush();

        std::print(m_all_out, "N");

        for (auto& s : m_args_headers)
            std::print(m_all_out, ", {}", s);

        std::print(m_all_out, ", MST_Cost, MST_Runtime");

        for (auto e : m_evaluators) {
            std::print(m_all_out,
                       ", {}_MFC_Cost, {}_MFC_Runtime, {}_Gamma, {}_Cluster_Size_Mu, {}_Cluster_Size_Sigma, {}_Sub_Clustering_Runtime, {}_Completion_Edges_Runtime, {}_Completion_Runtime, "
                       "{}_Clustering_Runtime",
                       e.first,
                       e.first,
                       e.first,
                       e.first,
                       e.first,
                       e.first,
                       e.first,
                       e.first,
                       e.first);
        }
        std::print(m_all_out, "\n");
        m_all_out.flush();

        return {};
    }

    ErrorOr<void> run_test(size_t repeats, const Args&... args) {

        struct EvaulatorResults {
            double mfc_cost;
            double mfc_runtime;

            double gamma;
            double cluster_size_mu;
            double cluster_size_sigma;

            double sub_cluster_runtime;
            double completion_edges_runtime;
            double completion_runtime;
            double clustering_runtime;
        };

        struct Results {
            size_t n;
            double mst_cost;
            double mst_runtime;

            std::vector<EvaulatorResults> evaluator_res;
        };

        auto execute_test = [&](std::vector<Point> points) -> Results {
            auto [mst, cur_mst_runtime] = time_code([&]() { return MST_Implicit(points, m_dist_func); });

            double cur_mst_cost = 0;
            for (auto& e : mst)
                cur_mst_cost += e.weight;

            std::vector<EvaulatorResults> evaluator_results;
            evaluator_results.reserve(m_evaluators.size());

            for (size_t j = 0; j < m_evaluators.size(); j++) {
                auto [evaluator_res, mfc_runtime] = time_code([&]() { return m_evaluators[j].second(points, args...); });
                auto [clustering, mfc] = std::move(evaluator_res);

                double mfc_cluster_weights = 0;
                for (auto c : mfc.cluster_edges)
                    for (auto e : c)
                        mfc_cluster_weights += e.weight;

                double mfc_cost = mfc_cluster_weights;
                for (auto e : mfc.completion_edges)
                    mfc_cost += e.weight;

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

                evaluator_results.push_back(EvaulatorResults{
                    .mfc_cost = mfc_cost,
                    .mfc_runtime = mfc_runtime,
                    .gamma = gamma,
                    .cluster_size_mu = cluster_size_mu,
                    .cluster_size_sigma = cluster_size_sigma,
                    .sub_cluster_runtime = mfc.sub_cluster_runtime,
                    .completion_edges_runtime = mfc.completion_edges_runtime,
                    .completion_runtime = mfc.completion_runtime,
                    .clustering_runtime = clustering.runtime,
                });
            }

            return Results{.n = points.size(), .mst_cost = cur_mst_cost, .mst_runtime = cur_mst_runtime, .evaluator_res = evaluator_results};
        };

        auto write_individual_results = [&](Results res) {
            std::print(m_all_out, "{}", res.n);
            (std::print(m_all_out, ", {}", args), ...);
            std::print(m_all_out, ", {}, {}", res.mst_cost, res.mst_runtime);

            for (size_t j = 0; j < m_evaluators.size(); j++) {
                EvaulatorResults& cur = res.evaluator_res[j];
                std::print(m_all_out,
                           ", {}, {}, {}, {}, {}, {}, {}, {}, {}",
                           cur.mfc_cost,
                           cur.mfc_runtime,
                           cur.gamma,
                           cur.cluster_size_mu,
                           cur.cluster_size_sigma,
                           cur.sub_cluster_runtime,
                           cur.completion_edges_runtime,
                           cur.completion_runtime,
                           cur.clustering_runtime);
            }

            std::print(m_all_out, "\n");
            m_all_out.flush();
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

        for (auto& res : results)
            write_individual_results(res);

        auto extract = [](auto vec, auto f) {
            std::vector<decltype(f(vec[0]))> res;
            for (auto& v : vec)
                res.push_back(f(v));
            return res;
        };

        auto [n_mu, n_sigma] = compute_stats(extract(results, [](Results& r) { return (double)r.n; }));
        auto [mst_cost_mu, mst_cost_sigma] = compute_stats(extract(results, [](Results& r) { return r.mst_cost; }));
        auto [mst_time_mu, mst_time_sigma] = compute_stats(extract(results, [](Results& r) { return r.mst_runtime; }));

        std::print(m_out, "{}, {}", n_mu, n_sigma);
        (std::print(m_out, ", {}", args), ...);
        std::print(m_out, ", {}, {}, {}, {}", mst_cost_mu, mst_cost_sigma, mst_time_mu, mst_time_sigma);

        for (size_t j = 0; j < m_evaluators.size(); j++) {
            auto p = [&](std::tuple<double, double> v) { std::print(m_out, ", {}, {}", std::get<0>(v), std::get<1>(v)); };

            p(compute_stats(extract(results, [&](Results& r) { return r.evaluator_res[j].mfc_cost; })));
            p(compute_stats(extract(results, [&](Results& r) { return r.evaluator_res[j].mfc_runtime; })));

            p(compute_stats(extract(results, [&](Results& r) { return r.evaluator_res[j].gamma; })));
            p(compute_stats(extract(results, [&](Results& r) { return r.evaluator_res[j].cluster_size_mu; })));
            p(compute_stats(extract(results, [&](Results& r) { return r.evaluator_res[j].cluster_size_sigma; })));

            p(compute_stats(extract(results, [&](Results& r) { return r.evaluator_res[j].sub_cluster_runtime; })));
            p(compute_stats(extract(results, [&](Results& r) { return r.evaluator_res[j].completion_edges_runtime; })));
            p(compute_stats(extract(results, [&](Results& r) { return r.evaluator_res[j].completion_runtime; })));
            p(compute_stats(extract(results, [&](Results& r) { return r.evaluator_res[j].clustering_runtime; })));
        }
        std::print(m_out, "\n");
        m_out.flush();

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
                    std::tuple<Clustering, MetricForestCompletion> Called in parallel when multithreading is enabled
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