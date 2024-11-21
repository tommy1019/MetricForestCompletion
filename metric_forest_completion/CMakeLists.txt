cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(metric_forest_completion VERSION 1.0 LANGUAGES CXX)

add_executable(uniform uniform.cpp)
add_executable(gaussian gaussian.cpp)

add_executable(gaussian_point_gen gaussian_point_gen.cpp)

add_executable(hdf5_784_dim_euclidean hdf5_784_dim_euclidean.cpp)
target_compile_options(hdf5_784_dim_euclidean PUBLIC "-Wno-stack-exhausted")
target_link_options(hdf5_784_dim_euclidean PUBLIC "LINKER:-stack_size,0x1000000")

add_executable(edit_distance edit_distance.cpp)
add_executable(hamming_distance hamming_distance.cpp)

add_executable(jaccard jaccard.cpp)
