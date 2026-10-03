#pragma once

#include "Graph.h"

#include <string>

struct ExperimentResult {
    std::string graph_type;
    std::string algorithm;
    int vertices;
    int edges;
    double runtime_ms;
    long peak_memory_kb;
};

ExperimentResult run_experiment(
    const Graph& graph,
    const std::string& graph_type,
    const std::string& algorithm,
    int source,
    int repetitions = 5
);