
#include "lib/args.h"
#include "lib/random_subset.h"

#include "common.h"

// Edit distance test

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

    // Run standard set of evalulators
    run_standard_evalulators<std::string>(args.output_file, args.all_output_file, args.cluster_test, gen_dataset, edit_dist);
}