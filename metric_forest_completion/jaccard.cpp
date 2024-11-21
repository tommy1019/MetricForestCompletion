
#include <set>

#include "lib/random_subset.h"
#include "lib/test_runner.h"

#include "algo/k_centering.h"

// Jaccard similarity distance test

int main(int argc, char** argv) {

    if (argc != 4 && argc != 5 && argc != 6) {
        std::print("Usage: {} [strings file] [out] [all_out] [edge_size_filter] [cluster_test]\n", argv[0]);
        exit(1);
    }

    size_t edge_size_filter = 0;
    if (argc == 5)
        edge_size_filter = std::stoi(argv[4]);

    bool CLUSTER_DETECTION_TEST = false;
    if (argc == 6 && std::string(argv[5]) == "cluster_test") {
        CLUSTER_DETECTION_TEST = true;
    }

    // Load dataset from txt file. One set per line of comma seperated integers
    std::vector<std::set<size_t>> dataset;
    {
        std::ifstream in(argv[1]);
        if (!in) {
            std::print("Error: Could not open file {}\n", argv[1]);
            exit(1);
        }

        std::string line;
        while (std::getline(in, line)) {
            std::set<size_t> cur;
            for (auto s : std::views::split(line, ',') | std::views::transform([](auto r) { return std::string(r.data(), r.size()); })) {
                cur.insert(std::stoi(s));
            }

            if (cur.size() < edge_size_filter)
                continue;

            dataset.push_back(cur);
        }
    }

    std::print("Loaded dataset of size {}\n", dataset.size());

    // Jaccard similarity distance
    constexpr static auto jaccard = [](std::set<size_t> a, std::set<size_t> b) -> float {
        std::set<size_t> intersection;
        set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::inserter(intersection, intersection.begin()));

        std::set<size_t> union_set;
        set_union(a.begin(), a.end(), b.begin(), b.end(), std::inserter(union_set, union_set.begin()));

        if (union_set.empty()) {
            return 1.0;
        }

        return 1.0f - (float)intersection.size() / union_set.size();
    };

    // Generate function for test runner. Returns a random size N subset from dataset
    auto gen_dataset = [&](std::default_random_engine& re, size_t N) -> ErrorOr<std::vector<std::set<size_t>>> { return random_subset(dataset, N, re); };

    // Generates a clustering evaluator for a given amount of clusters
    auto fixed_cluster = [](size_t clusters) -> std::pair<std::string, std::function<std::tuple<Clustering, MetricForestCompletion>(std::vector<std::set<size_t>>, size_t N)>> {
        return {"C" + std::to_string(clusters), [clusters](std::vector<std::set<size_t>> points, size_t N) {
                    auto clustering = k_centering(points, clusters, jaccard);
                    auto mfc = metric_forest_completion(points, clusters, clustering.assignments, jaccard);
                    return std::tuple{clustering, mfc};
                }};
    };

    // List of evaluators to run
    std::vector<std::pair<std::string, std::function<std::tuple<Clustering, MetricForestCompletion>(std::vector<std::set<size_t>>, size_t N)>>> evaluators = {{
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
        auto test_runner = MUST(CreateTestRunner<std::set<size_t>, true, size_t>(argv[2], argv[3], {"N"}, jaccard, gen_dataset, evaluators));
        MUST(test_runner.run_test(32, 20000));
    } else {
        // Create and run a test runner
        auto test_runner = MUST(CreateTestRunner<std::set<size_t>, true, size_t>(argv[2], argv[3], {"N"}, jaccard, gen_dataset, evaluators));
        MUST(test_runner.run_test(16, dataset.size()));
    }
}