#pragma once

#include "Graph.h"

#include <vector>

std::vector<std::vector<int>> connected_components(
    const Graph& graph
);

std::vector<int> one_cycle(
    const Graph& graph
);

std::vector<std::vector<int>> shortest_paths(
    const Graph& graph,
    int source
);