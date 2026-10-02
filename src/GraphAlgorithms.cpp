#include "GraphAlgorithms.h"

#include <functional>

std::vector<std::vector<int>> connected_components(
    const Graph& graph
) {
    const int n = graph.vertex_count();

    std::vector<bool> visited(n, false);
    std::vector<std::vector<int>> components;

    std::function<void(int, std::vector<int>&)> dfs =
        [&](int vertex, std::vector<int>& component) {
            visited[vertex] = true;
            component.push_back(vertex);

            for (int neighbor : graph.neighbors(vertex)) {
                if (!visited[neighbor]) {
                    dfs(neighbor, component);
                }
            }
        };

    for (int vertex = 0; vertex < n; ++vertex) {
        if (!visited[vertex]) {
            std::vector<int> component;
            dfs(vertex, component);
            components.push_back(component);
        }
    }

    return components;
}