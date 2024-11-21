
#include "lib/random_subset.h"
#include "lib/test_runner.h"

#include "algo/k_centering.h"

// Hamming distance test

int main(int argc, char** argv) {

    if (argc != 4 && argc != 5) {
        std::print("Usage: {} [strings file] [out] [all_out]\n", argv[0]);
        exit(1);
    }

    bool CLUSTER_DETECTION_TEST = false;
    if (argc == 5 && std::string(argv[4]) == "cluster_test") {
        CLUSTER_DETECTION_TEST = true;
    }

    // Load dataset from txt file. One string per line
    std::vector<std::string> dataset;
    {
        std::ifstream in(argv[1]);
        if (!in) {
            std::print("Error: Could not open file {}\n", argv[1]);
            exit(1);
        }

        std::string line;
        while (std::getline(in, line)) {
            dataset.push_back(line);
        }
    }

    std::print("Loaded dataset of size {}\n", dataset.size());

    // Distance function
    constexpr static auto hamming_distance = [](std::string a, std::string b) {
        size_t res = 0;

        REQUIRE(a.size() == b.size(), "Strings do not all have the same size\n")

        for (size_t i = 0; i < a.size(); i++) {
            if (a[i] != b[i])
                res++;
        }

        return (float)res;
    };

    // Generate function for test runner. Returns a random size N subset from dataset
    auto gen_dataset = [&](std::default_random_engine& re, size_t N) -> ErrorOr<std::vector<std::string>> { return random_subset(dataset, N, re); };

    // Generates a clustering evaluator for a given amount of clusters
    auto fixed_cluster = [](size_t clusters) -> std::pair<std::string, std::function<std::tuple<Clustering, MetricForestCompletion>(std::vector<std::string>, size_t N)>> {
        return {"C" + std::to_string(clusters), [clusters](std::vector<std::string> points, size_t N) {
                    auto clustering = k_centering(points, clusters, hamming_distance);
                    auto mfc = metric_forest_completion(points, clusters, clustering.assignments, hamming_distance);
                    return std::tuple{clustering, mfc};
                }};
    };

    // List of evaluators to run
    std::vector<std::pair<std::string, std::function<std::tuple<Clustering, MetricForestCompletion>(std::vector<std::string>, size_t N)>>> evaluators = {{
        fixed_cluster(16),
        fixed_cluster(32),
        fixed_cluster(64),
        fixed_cluster(128),
        fixed_cluster(256),
    }};

    if (CLUSTER_DETECTION_TEST) {
        // Replace the set evaluators with a list of every cluster amount from 2 to 150
        evaluators.clear();
        for (int i = 2; i < 150; i++) {
            evaluators.push_back(fixed_cluster(i));
        }

        // Create and run a test runner
        auto test_runner = MUST(CreateTestRunner<std::string, true, size_t>(argv[2], argv[3], {"N"}, hamming_distance, gen_dataset, evaluators));
        MUST(test_runner.run_test(32, 20000));
    } else {
        // Create and run a test runner
        auto test_runner = MUST(CreateTestRunner<std::string, true, size_t>(argv[2], argv[3], {"N"}, hamming_distance, gen_dataset, evaluators));

        for (int i = 500; i <= 30000; i += 100) {
            std::print("Running tests for N={}\n", i);
            MUST(test_runner.run_test(16, i));
        }
    }
}