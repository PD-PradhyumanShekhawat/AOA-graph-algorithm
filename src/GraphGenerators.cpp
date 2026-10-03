#include "GraphGenerators.h"

Graph generate_cycle_graph(int vertices) {
    Graph graph(vertices);

    if (vertices < 3) {
        return graph;
    }

    // Connect consecutive vertices in a cycle.
    for (int vertex = 0; vertex < vertices; ++vertex) {
        const int next = (vertex + 1) % vertices;
        graph.add_edge(vertex, next);
    }

    return graph;
}

Graph generate_complete_graph(int vertices) {
    Graph graph(vertices);

    // Connect every pair of distinct vertices.
    for (int u = 0; u < vertices; ++u) {
        for (int v = u + 1; v < vertices; ++v) {
            graph.add_edge(u, v);
        }
    }

    return graph;
}

Graph generate_empty_graph(int vertices) {
    // Create vertices without adding any edges.
    return Graph(vertices);
}