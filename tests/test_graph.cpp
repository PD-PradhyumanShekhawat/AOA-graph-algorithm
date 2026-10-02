#include "Graph.h"
#include "GraphAlgorithms.h"

#include <cassert>
#include <iostream>

int main() {
    // Connected components
    {
        Graph graph(6);

        graph.add_edge(0, 1);
        graph.add_edge(1, 2);
        graph.add_edge(3, 4);

        const auto components = connected_components(graph);

        assert(components.size() == 3);
    }

    // Acyclic graph
    {
        Graph graph(4);

        graph.add_edge(0, 1);
        graph.add_edge(1, 2);
        graph.add_edge(2, 3);

        const auto cycle = one_cycle(graph);

        assert(cycle.empty());
    }

    // Triangle
    {
        Graph graph(3);

        graph.add_edge(0, 1);
        graph.add_edge(1, 2);
        graph.add_edge(2, 0);

        const auto cycle = one_cycle(graph);

        assert(!cycle.empty());
        assert(cycle.size() >= 4);
        assert(cycle.front() == cycle.back());
    }

    // Graph containing multiple cycles
    {
        Graph graph(5);

        graph.add_edge(0, 1);
        graph.add_edge(1, 2);
        graph.add_edge(2, 0);

        graph.add_edge(2, 3);
        graph.add_edge(3, 4);
        graph.add_edge(4, 2);

        const auto cycle = one_cycle(graph);

        assert(!cycle.empty());
        assert(cycle.front() == cycle.back());
        assert(cycle.size() >= 4);
    }

    std::cout << "All graph algorithm tests passed.\n";

    return 0;
}