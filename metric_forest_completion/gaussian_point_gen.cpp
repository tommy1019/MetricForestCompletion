#include "lib/error.h"
#include "lib/vec.h"

#include <random>

// Test file to generate d dimensional gaussian point data for figures

template <size_t D>
void test_dim() {
    using Vec = Vec<float, D>;

    auto gen_dataset = [&](std::default_random_engine& re, size_t num_gauss, size_t points_per_gauss) -> ErrorOr<std::vector<Vec>> {
        std::vector<Vec> points;
        points.reserve(num_gauss * points_per_gauss);

        return points;
    };

    size_t num_gauss = 30;
    size_t points_per_gauss = 300;

    auto re = std::default_random_engine(std::random_device{}());

    std::uniform_real_distribution<> random_dist(-1, 1);

    constexpr auto mean_range = std::make_pair(-5.0, 5.0);
    constexpr auto sigma_range = std::make_pair(0.5, 0.8);

    printf("g, x, y\n");

    for (size_t i = 0; i < num_gauss; i++) {
        std::array<std::normal_distribution<>, D> dists;
        for (int d = 0; d < D; d++)
            dists[d] = std::normal_distribution{
                std::uniform_real_distribution{mean_range.first, mean_range.second}(re),
                std::uniform_real_distribution{sigma_range.first, sigma_range.second}(re),
            };

        for (size_t j = 0; j < points_per_gauss; j++) {
            printf("%lu", i);

            std::array<float, D> v;
            for (int d = 0; d < D; d++) {
                v[d] = dists[d](re);

                printf(", %f", v[d]);
            }
            printf("\n");
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::print("Usage: {} [dim_count]\n", argv[0]);
        exit(1);
    }

    size_t d = std::stoi(argv[1]);

#define DIM(D)                                                                                                                                                                                         \
    case D:                                                                                                                                                                                            \
        test_dim<D>();                                                                                                                                                                                 \
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