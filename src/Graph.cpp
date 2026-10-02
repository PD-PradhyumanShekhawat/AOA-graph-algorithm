#include "Graph.h"

#include <stdexcept>

Graph::Graph(int vertices)
    : vertices_(vertices),
      edges_(0),
      adjacency_(vertices) {
    if (vertices < 0) {
        throw std::invalid_argument(
            "Number of vertices cannot be negative."
        );
    }
}

void Graph::add_edge(int u, int v) {
    if (u < 0 || u >= vertices_ ||
        v < 0 || v >= vertices_) {
        throw std::out_of_range(
            "Vertex index out of range."
        );
    }

    if (u == v) {
        throw std::invalid_argument(
            "Self-loops are not allowed."
        );
    }

    adjacency_[u].push_back(v);
    adjacency_[v].push_back(u);

    ++edges_;
}

int Graph::vertex_count() const {
    return vertices_;
}

int Graph::edge_count() const {
    return edges_;
}

const std::vector<int>& Graph::neighbors(int vertex) const {
    if (vertex < 0 || vertex >= vertices_) {
        throw std::out_of_range(
            "Vertex index out of range."
        );
    }

    return adjacency_[vertex];
}