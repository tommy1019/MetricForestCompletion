#include <cfloat>

#include "lib/args.h"
#include "lib/test_runner.h"
#include "lib/vec.h"

#include "algo/k_centering.h"

#include "common.h"

// Uniform synthetic data test

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

    // Run standard set of evalulators
    run_standard_evalulators<Vec>(args.output_file, args.all_output_file, args.cluster_test, gen_dataset, dist_func);
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