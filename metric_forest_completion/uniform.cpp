#include "lib/test_runner.h"
#include "lib/vec.h"

#include "algo/k_centering.h"

// Uniform synthetic data test

bool CLUSTER_DETECTION_TEST = false;

// Templated function to run tests on the given dimension D
template <size_t D>
void test_dim(std::string out, std::string all_out) {
    using Vec = Vec<float, D>;

    // Euclidean distance
    constexpr static auto dist_func = [](const Vec& a, const Vec& b) { return (b - a).length(); };

    // Generate function for test runner. Generates N points
    auto gen_dataset = [&](std::default_random_engine& re, size_t N) -> ErrorOr<std::vector<Vec>> {
        std::vector<Vec> points;
        points.reserve(N);
        std::uniform_real_distribution<> random_dist(-1, 1);

        for (int i = 0; i < N; i++) {
            std::array<float, D> v;
            for (int i = 0; i < D; i++)
                v[i] = (float)random_dist(re);
            points.push_back(Vec{v});
        }
        return points;
    };

    // Generates a clustering evaluator for a given amount of clusters
    auto fixed_cluster = [](size_t clusters) -> std::pair<std::string, std::function<std::tuple<Clustering, MetricForestCompletion>(std::vector<Vec>, size_t N)>> {
        return {"C" + std::to_string(clusters), [clusters](std::vector<Vec> points, size_t N) {
                    auto clustering = k_centering(points, clusters, dist_func);
                    auto mfc = metric_forest_completion(points, clusters, clustering.assignments, dist_func);
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
        auto test_runner = MUST(CreateTestRunner<Vec, true, size_t>(out, all_out, std::array<std::string, 1>{"N"}, dist_func, gen_dataset, evaluators));
        MUST(test_runner.run_test(32, 20000));
    } else {
        // Create and run a test runner
        auto test_runner = MUST(CreateTestRunner<Vec, true, size_t>(out, all_out, std::array<std::string, 1>{"N"}, dist_func, gen_dataset, evaluators));
        for (int i = 500; i <= 30000; i += 100) {
            std::print("Running tests for N={}\n", i);
            MUST(test_runner.run_test(16, i));
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 4 && argc != 5) {
        std::print("Usage: {} [dim_count] [out] [all_out]\n", argv[0]);
        exit(1);
    }

    if (argc == 5 && std::string(argv[4]) == "cluster_test") {
        CLUSTER_DETECTION_TEST = true;
    }

    size_t d = std::stoi(argv[1]);

#define DIM(D)                                                                                                                                                                                         \
    case D:                                                                                                                                                                                            \
        test_dim<D>(argv[2], argv[3]);                                                                                                                                                                 \
        break;

    switch (d) {
        DIM(2);
        DIM(4);
        DIM(8);
        DIM(16);
        DIM(32);
        DIM(64);
        DIM(128);
        DIM(256);
        DIM(512);
    }
}