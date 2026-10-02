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

std::vector<int> one_cycle(const Graph& graph) {
    const int n = graph.vertex_count();

    std::vector<bool> visited(n, false);
    std::vector<int> parent(n, -1);
    std::vector<int> path;

    std::function<bool(int)> dfs = [&](int vertex) {
        visited[vertex] = true;
        path.push_back(vertex);

        for (int neighbor : graph.neighbors(vertex)) {
            if (neighbor == parent[vertex]) {
                continue;
            }

            if (!visited[neighbor]) {
                parent[neighbor] = vertex;

                if (dfs(neighbor)) {
                    return true;
                }
            } else {
                // Found an edge to an already visited vertex.
                // In an undirected graph, this indicates a cycle.
                path.push_back(neighbor);
                return true;
            }
        }

        path.pop_back();
        return false;
    };

    for (int vertex = 0; vertex < n; ++vertex) {
        if (!visited[vertex]) {
            if (dfs(vertex)) {
                const int cycle_start = path.back();

                std::vector<int> cycle;
                cycle.push_back(cycle_start);

                for (int i = static_cast<int>(path.size()) - 2;
                     i >= 0;
                     --i) {
                    cycle.push_back(path[i]);

                    if (path[i] == cycle_start) {
                        break;
                    }
                }

                cycle.push_back(cycle_start);

                return cycle;
            }
        }
    }

    return {};
}