#pragma once

#include <vector>

// Type representing a clustering as well as the time taken to cluster. assignments[i] represents the cluster index for the ith point
struct Clustering {
    std::vector<size_t> assignments;
    double runtime;
};