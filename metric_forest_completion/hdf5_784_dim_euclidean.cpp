
#include "lib/args.h"
#include "lib/hdf5.h"
#include "lib/random_subset.h"
#include "lib/vec.h"

#include "common.h"

// Euclidean distance for 784 dim vectors loaded from a HDF5 file

int main(int argc, char** argv) {

    struct {
        std::string input_file;
        std::string output_file;
        std::string all_output_file;
        bool cluster_test = false;
    } args;

    REQUIRE(parse_arg(argc, argv, "input_file", args.input_file, 'i'), "");
    REQUIRE(parse_arg(argc, argv, "output_file", args.output_file, 'o'), "");
    REQUIRE(parse_arg(argc, argv, "all_output_file", args.all_output_file, 'a'), "");
    REQUIRE(parse_arg(argc, argv, "cluster_test", args.cluster_test, 'c', false), "");

    // Vector type to be used
    using Vec = Vec<float, 784>;

    // Load dataset using HDF5 utility
    auto dataset = MUST(HDF5::load_data_set<Vec>(args.input_file, "train"));

    // Euclidean distance
    static constexpr auto dist_func = [](const Vec& a, const Vec& b) { return (b - a).length(); };

    // Generate function for test runner. Returns a random size N subset from dataset
    auto gen_dataset = [&](std::default_random_engine& re, size_t N) -> ErrorOr<std::vector<Vec>> { return random_subset(dataset, N, re); };

    // Run standard set of evalulators
    run_standard_evalulators<Vec>(args.output_file, args.all_output_file, args.cluster_test, gen_dataset, dist_func);
}