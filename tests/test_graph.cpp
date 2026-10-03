#include "Graph.h"
#include "GraphAlgorithms.h"
#include "GraphGenerators.h"

#include <cassert>
#include <iostream>
#include <vector>

bool is_valid_cycle(
    const Graph& graph,
    const std::vector<int>& cycle
) {
    // A valid cycle must contain at least three distinct edges,
    // so the returned representation has at least four vertices:
    // for example, 0 -> 1 -> 2 -> 0.
    if (cycle.size() < 4) {
        return false;
    }

    // The cycle must start and end at the same vertex.
    if (cycle.front() != cycle.back()) {
        return false;
    }

    // Verify that every consecutive pair of vertices is an edge.
    for (std::size_t i = 0; i + 1 < cycle.size(); ++i) {
        const int vertex = cycle[i];
        const int next = cycle[i + 1];

        const auto& neighbors = graph.neighbors(vertex);

        bool edge_exists = false;

        for (int neighbor : neighbors) {
            if (neighbor == next) {
                edge_exists = true;
                break;
            }
        }

        if (!edge_exists) {
            return false;
        }
    }

    return true;
}

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

        assert(is_valid_cycle(graph, cycle));
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
        assert(is_valid_cycle(graph, cycle));
    }

    // Dijkstra shortest paths
    {
        Graph graph(6);

        graph.add_edge(0, 1);
        graph.add_edge(0, 2);
        graph.add_edge(1, 3);
        graph.add_edge(2, 3);
        graph.add_edge(3, 4);

        const auto paths = shortest_paths(graph, 0);

        // Source-to-source path must contain only the source.
        assert(paths[0] == std::vector<int>{0});

        // Verify paths contain valid shortest routes.
        bool found_01 = false;
        bool found_02 = false;
        bool found_03 = false;
        bool found_04 = false;

        for (const auto& path : paths) {
            if (path == std::vector<int>{0, 1}) {
                found_01 = true;
            }

            if (path == std::vector<int>{0, 2}) {
                found_02 = true;
            }

            if (path == std::vector<int>{0, 1, 3} ||
                path == std::vector<int>{0, 2, 3}) {
                found_03 = true;
            }

            if (path == std::vector<int>{0, 1, 3, 4} ||
                path == std::vector<int>{0, 2, 3, 4}) {
                found_04 = true;
            }
        }

        assert(found_01);
        assert(found_02);
        assert(found_03);
        assert(found_04);

        // Vertex 5 is unreachable and must not appear.
        for (const auto& path : paths) {
            assert(path.back() != 5);
        }
    }

    // Single-vertex graph
    {
        Graph graph(1);

        const auto paths = shortest_paths(graph, 0);

        assert(paths.size() == 1);
        assert(paths[0] == std::vector<int>{0});
    }

    // Graph generators
    {
        const Graph cycle = generate_cycle_graph(5);

        assert(cycle.vertex_count() == 5);
        assert(cycle.edge_count() == 5);
    }

    {
        const Graph complete = generate_complete_graph(5);

        assert(complete.vertex_count() == 5);
        assert(complete.edge_count() == 10);
    }

    {
        const Graph empty = generate_empty_graph(5);

        assert(empty.vertex_count() == 5);
        assert(empty.edge_count() == 0);
    }

    std::cout << "All graph algorithm tests passed.\n";

    return 0;
}