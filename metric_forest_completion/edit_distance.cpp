
#include "lib/random_subset.h"
#include "lib/test_runner.h"

#include "algo/k_centering.h"

// Edit distance test

int main(int argc, char** argv) {

    if (argc != 5 && argc != 6) {
        std::print("Usage: {} [strings file] [out] [all_out] [n]\n", argv[0]);
        exit(1);
    }

    bool CLUSTER_DETECTION_TEST = false;
    if (argc == 6 && std::string(argv[5]) == "cluster_test") {
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

    // https://en.wikipedia.org/wiki/Levenshtein_distance
    constexpr static auto edit_dist = [](std::string a, std::string b) {
        struct {
            size_t* d_mem;
            size_t row_len;
            size_t* operator[](size_t i) { return d_mem + (i * row_len); }
        } d;

        d.d_mem = (size_t*)malloc(sizeof(size_t) * (a.size() + 1) * (b.size() + 1));

        if (d.d_mem == nullptr)
            std::print("ERROR: Could not allocate memory");

        d.row_len = (b.size() + 1);

        memset(d.d_mem, 0, sizeof(size_t) * (a.size() + 1) * (b.size() + 1));

        for (size_t i = 1; i <= a.size(); i++)
            d[i][0] = i;

        for (size_t i = 1; i <= b.size(); i++)
            d[0][i] = i;

        for (size_t j = 1; j <= b.size(); j++)
            for (size_t i = 1; i <= a.size(); i++) {
                size_t substitutionCost;
                if (a[i - 1] == b[j - 1]) {
                    substitutionCost = 0;
                } else {
                    substitutionCost = 1;
                }

                d[i][j] = std::min(d[i - 1][j] + 1,                               // deletion
                                   std::min(d[i][j - 1] + 1,                      // insertion
                                            d[i - 1][j - 1] + substitutionCost)); // substitution
            }

        auto res = (float)d[a.size()][b.size()];

        free(d.d_mem);

        return res;
    };

    // Generate function for test runner. Returns a random size N subset from dataset
    auto gen_dataset = [&](std::default_random_engine& re, size_t N) -> ErrorOr<std::vector<std::string>> { return random_subset(dataset, N, re); };

    // Generates a clustering evaluator for a given amount of clusters
    auto fixed_cluster = [](size_t clusters) -> std::pair<std::string, std::function<std::tuple<Clustering, MetricForestCompletion>(std::vector<std::string>, size_t N)>> {
        return {"C" + std::to_string(clusters), [clusters](std::vector<std::string> points, size_t N) {
                    auto cluster_assignments = k_centering(points, clusters, edit_dist);
                    auto mfc = metric_forest_completion(points, clusters, cluster_assignments.assignments, edit_dist);
                    return std::tuple{cluster_assignments, mfc};
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
        auto test_runner = MUST(CreateTestRunner<std::string, true, size_t>(argv[2], argv[3], {"N"}, edit_dist, gen_dataset, evaluators));
        MUST(test_runner.run_test(32, 20000));
    } else {
        // Create and run a test runner
        auto test_runner = MUST(CreateTestRunner<std::string, true, size_t>(argv[2], argv[3], {"N"}, edit_dist, gen_dataset, evaluators));
        MUST(test_runner.run_test(16, std::stod(argv[4])));
    }

    // for (int i = 500; i <= dataset.size(); i += 100) {
    //     std::print("Running tests for N={}\n", i);
    //     MUST(test_runner.run_test(16, i));
    // }
}