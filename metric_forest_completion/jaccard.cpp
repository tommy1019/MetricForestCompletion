
#include <set>

#include "lib/args.h"
#include "lib/random_subset.h"

#include "common.h"

// Jaccard similarity distance test

int main(int argc, char** argv) {

    struct {
        std::string input_file;
        std::string output_file;
        std::string all_output_file;
        bool cluster_test = false;
        int edge_size_filter;
    } args;

    REQUIRE(parse_arg(argc, argv, "input_file", args.input_file, 'i'), "");
    REQUIRE(parse_arg(argc, argv, "output_file", args.output_file, 'o'), "");
    REQUIRE(parse_arg(argc, argv, "all_output_file", args.all_output_file, 'a'), "");
    REQUIRE(parse_arg(argc, argv, "cluster_test", args.cluster_test, 'c', false), "");
    REQUIRE(parse_arg(argc, argv, "edge_size_filter", args.edge_size_filter, 'e'), "");

    // Load dataset from txt file. One set per line of comma seperated integers
    std::vector<std::set<size_t>> dataset;
    {
        std::ifstream in(args.input_file);
        if (!in) {
            std::print("Error: Could not open file {}\n", args.input_file);
            exit(1);
        }

        std::string line;
        while (std::getline(in, line)) {
            std::set<size_t> cur;
            for (auto s : std::views::split(line, ',') | std::views::transform([](auto r) { return std::string(r.data(), r.size()); })) {
                cur.insert(std::stoi(s));
            }

            if (cur.size() < args.edge_size_filter)
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

    // Run standard set of evalulators
    run_standard_evalulators<std::set<size_t>>(args.output_file, args.all_output_file, args.cluster_test, gen_dataset, jaccard, dataset.size());
}