#include "GraphAlgorithms.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <queue>
#include <stdexcept>
#include <vector>


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
    std::vector<int> cycle;

    std::function<bool(int)> dfs = [&](int vertex) {
        visited[vertex] = true;

        for (int neighbor : graph.neighbors(vertex)) {
            // Ignore the edge back to the immediate parent.
            if (neighbor == parent[vertex]) {
                continue;
            }

            if (!visited[neighbor]) {
                parent[neighbor] = vertex;

                if (dfs(neighbor)) {
                    return true;
                }
            } else {
                // Found a visited non-parent neighbor.
                // Reconstruct:
                //
                // neighbor -> ... -> vertex -> neighbor
                cycle.push_back(neighbor);

                int current = vertex;

                while (current != neighbor) {
                    cycle.push_back(current);
                    current = parent[current];
                }

                cycle.push_back(neighbor);

                return true;
            }
        }

        return false;
    };

    for (int vertex = 0; vertex < n; ++vertex) {
        if (!visited[vertex]) {
            if (dfs(vertex)) {
                return cycle;
            }
        }
    }

    return {};
}


std::vector<std::vector<int>> shortest_paths(
    const Graph& graph,
    int source
) {
    const int n = graph.vertex_count();

    if (source < 0 || source >= n) {
        throw std::out_of_range(
            "Source vertex is out of range."
        );
    }

    const int infinity = std::numeric_limits<int>::max();

    std::vector<int> distance(n, infinity);
    std::vector<int> predecessor(n, -1);

    using QueueEntry = std::pair<int, int>;

    std::priority_queue<
        QueueEntry,
        std::vector<QueueEntry>,
        std::greater<QueueEntry>
    > priority_queue;

    distance[source] = 0;
    priority_queue.push({0, source});

    while (!priority_queue.empty()) {
        const auto [current_distance, vertex] =
            priority_queue.top();

        priority_queue.pop();

        // Ignore outdated entries.
        if (current_distance != distance[vertex]) {
            continue;
        }

        for (int neighbor : graph.neighbors(vertex)) {
            const int new_distance = current_distance + 1;

            if (new_distance < distance[neighbor]) {
                distance[neighbor] = new_distance;
                predecessor[neighbor] = vertex;

                priority_queue.push({
                    new_distance,
                    neighbor
                });
            }
        }
    }

    std::vector<std::vector<int>> paths;

    for (int destination = 0; destination < n; ++destination) {
        if (distance[destination] == infinity) {
            continue;
        }

        std::vector<int> path;

        int current = destination;

        while (current != -1) {
            path.push_back(current);

            if (current == source) {
                break;
            }

            current = predecessor[current];
        }

        std::reverse(path.begin(), path.end());

        paths.push_back(path);
    }

    return paths;
}