#pragma once

#include <vector>

class Graph {
public:
    explicit Graph(int vertices);

    void add_edge(int u, int v);

    int vertex_count() const;
    int edge_count() const;

    const std::vector<int>& neighbors(int vertex) const;

private:
    int vertices_;
    int edges_;
    std::vector<std::vector<int>> adjacency_;
};