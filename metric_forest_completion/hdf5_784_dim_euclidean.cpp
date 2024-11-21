
#include "lib/hdf5.h"
#include "lib/random_subset.h"
#include "lib/test_runner.h"
#include "lib/vec.h"

#include "algo/k_centering.h"

// Euclidean distance for 784 dim vectors loaded from a HDF5 file

int main(int argc, char** argv) {

    if (argc != 4 && argc != 5) {
        std::print("Usage: {} [hdf file] [out] [all_out] [cluster_test]\n", argv[0]);
        exit(1);
    }

    bool CLUSTER_DETECTION_TEST = false;
    if (argc == 5 && std::string(argv[4]) == "cluster_test") {
        CLUSTER_DETECTION_TEST = true;
    }

    // Vector type to be used
    using Vec = Vec<float, 784>;

    // Load dataset using HDF5 utility
    auto dataset = MUST(HDF5::load_data_set<Vec>(argv[1], "train"));

    // Euclidean distance
    static constexpr auto dist_func = [](const Vec& a, const Vec& b) { return (b - a).length(); };

    // Generate function for test runner. Returns a random size N subset from dataset
    auto gen_dataset = [&](std::default_random_engine& re, size_t N) -> ErrorOr<std::vector<Vec>> { return random_subset(dataset, N, re); };

    // Generates a clustering evaluator for a given amount of clusters
    auto fixed_cluster = [CLUSTER_DETECTION_TEST](size_t clusters) -> std::pair<std::string, std::function<std::tuple<Clustering, MetricForestCompletion>(std::vector<Vec>, size_t N)>> {
        return {"C" + std::to_string(clusters), [clusters, CLUSTER_DETECTION_TEST](std::vector<Vec> points, size_t N) {
                    auto clustering = k_centering(points, clusters, dist_func);
                    auto mfc = metric_forest_completion(points, clusters, clustering.assignments, dist_func);
                    if (CLUSTER_DETECTION_TEST)
                        std::print("    Finished cluster: {}\n", clusters);
                    return std::tuple{clustering, mfc};
                }};
    };

    // List of evaluators to run
    std::vector<std::pair<std::string, std::function<std::tuple<Clustering, MetricForestCompletion>(std::vector<Vec>, size_t N)>>> evaluators = {{
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
        auto test_runner = MUST(CreateTestRunner<Vec, false, size_t>(argv[2], argv[3], {"N"}, dist_func, gen_dataset, evaluators));
        MUST(test_runner.run_test(32, 20000));
    } else {
        auto test_runner = MUST(CreateTestRunner<Vec, true, size_t>(argv[2], argv[3], {"N"}, dist_func, gen_dataset, evaluators));

        // Create and run a test runner
        for (int i = 500; i <= 30000; i += 100) {
            std::print("Running tests for N={}\n", i);
            MUST(test_runner.run_test(16, i));
        }
    }
}