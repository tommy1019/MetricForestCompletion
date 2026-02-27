#include "lib/args.h"
#include "lib/random_subset.h"

#include "common.h"

// Hamming distance test

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

    // Load dataset from txt file. One string per line
    std::vector<std::string> dataset;
    {
        std::ifstream in(args.input_file);
        if (!in) {
            std::print("Error: Could not open file {}\n", args.input_file);
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

    // Run standard set of evalulators
    run_standard_evalulators<std::string>(args.output_file, args.all_output_file, args.cluster_test, gen_dataset, hamming_distance);
}