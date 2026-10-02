#include "Graph.h"
#include "GraphAlgorithms.h"

#include <cassert>
#include <iostream>

int main() {
    Graph graph(6);

    graph.add_edge(0, 1);
    graph.add_edge(1, 2);

    graph.add_edge(3, 4);

    const auto components = connected_components(graph);

    assert(components.size() == 3);

    assert(components[0].size() == 3);
    assert(components[1].size() == 2);
    assert(components[2].size() == 1);

    std::cout << "Connected components test passed.\n";

    return 0;
}