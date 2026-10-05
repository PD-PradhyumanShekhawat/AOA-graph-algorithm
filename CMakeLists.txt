cmake_minimum_required(VERSION 3.16)

project(cot5405_graph_algorithms
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(graph
    src/Graph.cpp
    src/GraphAlgorithms.cpp
    src/GraphGenerators.cpp
)

target_include_directories(graph
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

add_executable(graph_test
    tests/test_graph.cpp
)

target_link_libraries(graph_test
    PRIVATE graph
)