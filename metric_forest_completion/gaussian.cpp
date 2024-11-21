#include "lib/test_runner.h"
#include "lib/vec.h"

#include "algo/k_centering.h"
#include "algo/metric_forest_completion.h"

// Gaussian synthetic data test

bool CLUSTER_DETECTION_TEST = false;

// Templated function to run tests on the given dimension D
template <size_t D>
void test_dim(std::string out, std::string all_out) {
    using Vec = Vec<float, D>;

    // Euclidean distance
    constexpr static auto dist_func = [](const Vec& a, const Vec& b) { return (b - a).length(); };

    // Generate function for test runner. Generates (num_gauss) gaussians with (points_per_gauss) points in each one
    auto gen_dataset = [&](std::default_random_engine& re, size_t num_gauss, size_t points_per_gauss) -> ErrorOr<std::vector<Vec>> {
        std::vector<Vec> points;
        points.reserve(num_gauss * points_per_gauss);

        std::uniform_real_distribution<> random_dist(-1, 1);

        constexpr auto mean_range = std::make_pair(-5.0, 5.0);
        constexpr auto sigma_range = std::make_pair(0.5, 0.8);

        for (size_t i = 0; i < num_gauss; i++) {
            std::array<std::normal_distribution<>, D> dists;
            for (int d = 0; d < D; d++)
                dists[d] = std::normal_distribution{
                    std::uniform_real_distribution{mean_range.first, mean_range.second}(re),
                    std::uniform_real_distribution{sigma_range.first, sigma_range.second}(re),
                };

            for (size_t j = 0; j < points_per_gauss; j++) {
                std::array<float, D> v;
                for (int d = 0; d < D; d++)
                    v[d] = dists[d](re);
                points.push_back(Vec{v});
            }
        }

        return points;
    };

    // Generates a clustering evaluator for a given amount of clusters
    auto fixed_cluster = [](size_t clusters) -> std::pair<std::string, std::function<std::tuple<Clustering, MetricForestCompletion>(std::vector<Vec>, size_t num_gauss, size_t points_per_gauss)>> {
        return {"C" + std::to_string(clusters), [clusters](std::vector<Vec> points, size_t num_gauss, size_t points_per_gauss) {
                    auto clustering = k_centering(points, clusters, dist_func);
                    auto mfc = metric_forest_completion(points, clusters, clustering.assignments, dist_func);
                    return std::tuple{clustering, mfc};
                }};
    };

    // List of evaluators to run
    std::vector<std::pair<std::string, std::function<std::tuple<Clustering, MetricForestCompletion>(std::vector<Vec>, size_t num_gauss, size_t points_per_gauss)>>> evaluators = {{
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
        auto test_runner = MUST(CreateTestRunner<Vec, true, size_t, size_t>(out, all_out, std::array<std::string, 2>{"GaussCount", "PointsPerGauss"}, dist_func, gen_dataset, evaluators));
        MUST(test_runner.run_test(32, 100, 200));
    } else {
        // Create and run a test runner
        auto test_runner = MUST(CreateTestRunner<Vec, true, size_t, size_t>(out, all_out, std::array<std::string, 2>{"GaussCount", "PointsPerGauss"}, dist_func, gen_dataset, evaluators));

        size_t N = 20000;

        for (size_t gauss = 8; gauss <= 300; gauss++) {
            size_t ppg = N / gauss;
            std::print("Running tests for num_gauss={}, points_per_gauss={}\n", gauss, ppg);
            MUST(test_runner.run_test(16, gauss, ppg));
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