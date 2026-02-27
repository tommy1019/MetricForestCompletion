#include "lib/args.h"
#include "lib/test_runner.h"
#include "lib/vec.h"

#include "algo/k_centering.h"
#include "algo/metric_forest_completion.h"

// Gaussian synthetic data test

struct {
    std::string input_file;
    std::string output_file;
    std::string all_output_file;
    bool cluster_test = false;
    int dim;
} args;

// Templated function to run tests on the given dimension D
template <size_t D>
void test_dim() {
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
    auto fixed_cluster = [](size_t clusters) -> std::pair<std::string, EvaluatorType<Vec, size_t, size_t>> {
        return {"C" + std::to_string(clusters), [clusters](std::vector<Vec> points, size_t num_gauss, size_t points_per_gauss) -> EvaluatorReturnType {
                    auto clustering = k_centering(points, clusters, dist_func);
                    auto mfc = metric_forest_completion(points, clusters, clustering.assignments, dist_func);
                    co_yield std::make_pair("normal", std::tuple{clustering, mfc});
                }};
    };

    // List of evaluators to run
    std::vector<std::pair<std::string, EvaluatorType<Vec, size_t, size_t>>> evaluators = {{
        fixed_cluster(16),
        fixed_cluster(32),
        fixed_cluster(64),
        fixed_cluster(128),
        fixed_cluster(256),
    }};

    if (args.cluster_test) {
        // Replace the set evaluators with a list of every cluster amount from 2 to 150
        evaluators.clear();
        for (int i = 2; i < 150; i++) {
            evaluators.push_back(fixed_cluster(i));
        }

        // Create and run a test runner
        auto test_runner
            = MUST(CreateTestRunner<Vec, true, size_t, size_t>(args.output_file, args.all_output_file, std::array<std::string, 2>{"GaussCount", "PointsPerGauss"}, dist_func, gen_dataset, evaluators));
        MUST(test_runner.run_test(32, 100, 200));
    } else {
        // Create and run a test runner
        auto test_runner
            = MUST(CreateTestRunner<Vec, true, size_t, size_t>(args.output_file, args.all_output_file, std::array<std::string, 2>{"GaussCount", "PointsPerGauss"}, dist_func, gen_dataset, evaluators));

        size_t N = 20000;

        for (size_t gauss = 8; gauss <= 300; gauss++) {
            size_t ppg = N / gauss;
            std::print("Running tests for num_gauss={}, points_per_gauss={}\n", gauss, ppg);
            MUST(test_runner.run_test(16, gauss, ppg));
        }
    }
}

int main(int argc, char** argv) {

    REQUIRE(parse_arg(argc, argv, "dimension", args.dim, 'd'), "");
    REQUIRE(parse_arg(argc, argv, "output_file", args.output_file, 'o'), "");
    REQUIRE(parse_arg(argc, argv, "all_output_file", args.all_output_file, 'a'), "");
    REQUIRE(parse_arg(argc, argv, "cluster_test", args.cluster_test, 'c', false), "");

#define DIM(D)                                                                                                                                                                                         \
    case D:                                                                                                                                                                                            \
        test_dim<D>();                                                                                                                                                                                 \
        break;

    switch (args.dim) {
        DIM(2);
        DIM(4);
        DIM(8);
        DIM(16);
        DIM(32);
        DIM(64);
        DIM(128);
        DIM(256);
        DIM(512);
    default:
        REQUIRE_NOT_REACHED("UNKNOWN DIMENSION: %d", args.dim)
    }
}