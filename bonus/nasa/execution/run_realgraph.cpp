#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

struct CSRGraph {
    uint64_t vertices = 0;
    uint64_t edges = 0;

    std::vector<uint64_t> offsets;
    std::vector<uint32_t> neighbors;

    void load(const std::string& path) {
        std::ifstream in(path, std::ios::binary);

        if (!in) {
            throw std::runtime_error(
                "Cannot open graph: " + path);
        }

        /*
         * Exact binary format written by build_nasa_graph.cpp:
         *
         * uint32_t vertices
         * uint64_t edge_count
         * uint64_t offsets[vertices + 1]
         * uint32_t neighbors[2 * edge_count]
         */

        uint32_t vertices32 = 0;
        uint64_t edge_count64 = 0;

        in.read(
            reinterpret_cast<char*>(&vertices32),
            sizeof(vertices32));

        in.read(
            reinterpret_cast<char*>(&edge_count64),
            sizeof(edge_count64));

        if (!in) {
            throw std::runtime_error(
                "Failed reading graph header.");
        }

        vertices = vertices32;
        edges = edge_count64;

        if (vertices == 0) {
            throw std::runtime_error(
                "Graph contains zero vertices.");
        }

        offsets.resize(vertices + 1);

        in.read(
            reinterpret_cast<char*>(offsets.data()),
            offsets.size() * sizeof(uint64_t));

        if (!in) {
            throw std::runtime_error(
                "Failed reading CSR offsets.");
        }

        const uint64_t neighbor_count = 2 * edges;

        neighbors.resize(neighbor_count);

        in.read(
            reinterpret_cast<char*>(neighbors.data()),
            neighbors.size() * sizeof(uint32_t));

        if (!in) {
            throw std::runtime_error(
                "Failed reading CSR neighbors.");
        }

        validate();
    }

    void validate() const {
        if (offsets.size() != vertices + 1) {
            throw std::runtime_error(
                "Invalid CSR offset count.");
        }

        if (offsets[0] != 0) {
            throw std::runtime_error(
                "CSR offsets[0] must be zero.");
        }

        if (offsets.back() != neighbors.size()) {
            std::ostringstream out;

            out << "CSR offsets/neighbor count mismatch: "
                << "offsets.back()="
                << offsets.back()
                << ", neighbors="
                << neighbors.size();

            throw std::runtime_error(out.str());
        }

        for (uint64_t i = 1;
             i < offsets.size();
             ++i) {

            if (offsets[i] < offsets[i - 1]) {
                throw std::runtime_error(
                    "CSR offsets are not nondecreasing.");
            }
        }

        for (uint32_t neighbor : neighbors) {
            if (neighbor >= vertices) {
                throw std::runtime_error(
                    "CSR neighbor ID is outside "
                    "the vertex range.");
            }
        }

        if (neighbors.size() != 2 * edges) {
            throw std::runtime_error(
                "Undirected edge count does not "
                "match CSR neighbor count.");
        }
    }

    uint64_t degree(uint32_t vertex) const {
        return offsets[vertex + 1] - offsets[vertex];
    }
};


static uint64_t peak_rss_bytes() {
    struct rusage usage {};

    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        return 0;
    }

#if defined(__APPLE__)
    return static_cast<uint64_t>(usage.ru_maxrss);
#else
    return static_cast<uint64_t>(usage.ru_maxrss) * 1024ULL;
#endif
}


static std::string format_bytes(uint64_t bytes) {
    const double gib =
        static_cast<double>(bytes) /
        (1024.0 * 1024.0 * 1024.0);

    std::ostringstream out;

    out << std::fixed
        << std::setprecision(3)
        << gib
        << " GiB";

    return out.str();
}


struct ComponentResult {
    uint64_t count = 0;
    uint64_t largest = 0;
};


static ComponentResult connected_components(
    const CSRGraph& graph) {

    std::vector<uint8_t> visited(
        graph.vertices,
        0);

    std::vector<uint32_t> stack;

    ComponentResult result;

    for (uint32_t start = 0;
         start < graph.vertices;
         ++start) {

        if (visited[start]) {
            continue;
        }

        ++result.count;

        uint64_t component_size = 0;

        stack.clear();
        stack.push_back(start);
        visited[start] = 1;

        while (!stack.empty()) {
            const uint32_t vertex =
                stack.back();

            stack.pop_back();

            ++component_size;

            const uint64_t begin =
                graph.offsets[vertex];

            const uint64_t end =
                graph.offsets[vertex + 1];

            for (uint64_t i = begin;
                 i < end;
                 ++i) {

                const uint32_t neighbor =
                    graph.neighbors[i];

                if (!visited[neighbor]) {
                    visited[neighbor] = 1;
                    stack.push_back(neighbor);
                }
            }
        }

        result.largest =
            std::max(
                result.largest,
                component_size);
    }

    return result;
}


/*
 * Iterative DFS cycle detection for an undirected graph.
 *
 * A cycle is reconstructed only when an edge points
 * from the current vertex to a strict DFS ancestor.
 */
static std::vector<uint32_t> one_cycle(
    const CSRGraph& graph) {

    std::vector<uint8_t> visited(
        graph.vertices,
        0);

    std::vector<uint32_t> parent(
        graph.vertices,
        std::numeric_limits<uint32_t>::max());

    std::vector<uint32_t> depth(
        graph.vertices,
        0);

    struct Frame {
        uint32_t vertex;
        uint64_t next_edge;
    };

    std::vector<Frame> stack;

    for (uint32_t start = 0;
         start < graph.vertices;
         ++start) {

        if (visited[start]) {
            continue;
        }

        visited[start] = 1;

        parent[start] =
            std::numeric_limits<uint32_t>::max();

        depth[start] = 0;

        stack.clear();

        stack.push_back({
            start,
            graph.offsets[start]
        });

        while (!stack.empty()) {
            Frame& frame = stack.back();

            const uint32_t vertex =
                frame.vertex;

            const uint64_t end =
                graph.offsets[vertex + 1];

            if (frame.next_edge >= end) {
                stack.pop_back();
                continue;
            }

            const uint32_t neighbor =
                graph.neighbors[
                    frame.next_edge++
                ];

            if (neighbor == parent[vertex]) {
                continue;
            }

            if (!visited[neighbor]) {
                parent[neighbor] = vertex;

                depth[neighbor] =
                    depth[vertex] + 1;

                visited[neighbor] = 1;

                stack.push_back({
                    neighbor,
                    graph.offsets[neighbor]
                });

                continue;
            }

            /*
             * Only a strict ancestor can produce
             * the back edge used for cycle reconstruction.
             */
            if (depth[neighbor] >= depth[vertex]) {
                continue;
            }

            std::vector<uint32_t> cycle;

            cycle.push_back(neighbor);

            uint32_t current = vertex;

            while (current != neighbor) {
                cycle.push_back(current);

                const uint32_t next =
                    parent[current];

                if (next ==
                    std::numeric_limits<uint32_t>::max()) {

                    cycle.clear();
                    break;
                }

                current = next;
            }

            if (!cycle.empty()) {
                cycle.push_back(neighbor);

                if (cycle.size() >= 4) {
                    return cycle;
                }
            }
        }
    }

    return {};
}


struct DijkstraResult {
    uint64_t reachable = 0;
    uint64_t max_distance = 0;
};


static DijkstraResult shortest_paths(
    const CSRGraph& graph,
    uint32_t source) {

    const uint64_t INF =
        std::numeric_limits<uint64_t>::max();

    std::vector<uint64_t> distance(
        graph.vertices,
        INF);

    using QueueEntry =
        std::pair<uint64_t, uint32_t>;

    std::priority_queue<
        QueueEntry,
        std::vector<QueueEntry>,
        std::greater<QueueEntry>> pq;

    distance[source] = 0;

    pq.push({
        0,
        source
    });

    DijkstraResult result;

    while (!pq.empty()) {
        const auto [current_distance, vertex] =
            pq.top();

        pq.pop();

        if (current_distance !=
            distance[vertex]) {

            continue;
        }

        ++result.reachable;

        result.max_distance =
            std::max(
                result.max_distance,
                current_distance);

        const uint64_t begin =
            graph.offsets[vertex];

        const uint64_t end =
            graph.offsets[vertex + 1];

        for (uint64_t i = begin;
             i < end;
             ++i) {

            const uint32_t neighbor =
                graph.neighbors[i];

            const uint64_t new_distance =
                current_distance + 1;

            if (new_distance <
                distance[neighbor]) {

                distance[neighbor] =
                    new_distance;

                pq.push({
                    new_distance,
                    neighbor
                });
            }
        }
    }

    return result;
}


struct ExperimentResult {
    double seconds = 0.0;
    uint64_t peak_rss = 0;
    std::string summary;
};


template <typename Function>
static ExperimentResult run_isolated(
    Function&& function) {

    int pipe_fds[2];

    if (pipe(pipe_fds) != 0) {
        throw std::runtime_error(
            "pipe() failed.");
    }

    const pid_t pid = fork();

    if (pid < 0) {
        close(pipe_fds[0]);
        close(pipe_fds[1]);

        throw std::runtime_error(
            "fork() failed.");
    }

    if (pid == 0) {
        close(pipe_fds[0]);

        const auto start =
            std::chrono::steady_clock::now();

        try {
            const std::string summary =
                function();

            const auto finish =
                std::chrono::steady_clock::now();

            const double seconds =
                std::chrono::duration<double>(
                    finish - start).count();

            const uint64_t rss =
                peak_rss_bytes();

            std::ostringstream message;

            message << std::setprecision(12)
                    << seconds
                    << '\n'
                    << rss
                    << '\n'
                    << summary;

            const std::string output =
                message.str();

            const ssize_t written =
                write(
                    pipe_fds[1],
                    output.data(),
                    output.size());

            (void)written;

            close(pipe_fds[1]);

            _exit(EXIT_SUCCESS);

        } catch (const std::exception& e) {

            const std::string error =
                std::string("ERROR: ") +
                e.what();

            write(
                pipe_fds[1],
                error.data(),
                error.size());

            close(pipe_fds[1]);

            _exit(EXIT_FAILURE);
        }
    }

    close(pipe_fds[1]);

    std::string output;

    char buffer[4096];

    while (true) {
        const ssize_t n =
            read(
                pipe_fds[0],
                buffer,
                sizeof(buffer));

        if (n <= 0) {
            break;
        }

        output.append(
            buffer,
            static_cast<size_t>(n));
    }

    close(pipe_fds[0]);

    int status = 0;

    if (waitpid(
            pid,
            &status,
            0) < 0) {

        throw std::runtime_error(
            "waitpid() failed.");
    }

    if (!WIFEXITED(status) ||
        WEXITSTATUS(status) != EXIT_SUCCESS) {

        throw std::runtime_error(
            output.empty()
                ? "Child experiment failed."
                : output);
    }

    std::istringstream input(output);

    ExperimentResult result;

    input >> result.seconds;
    input >> result.peak_rss;

    input.ignore(
        std::numeric_limits<
            std::streamsize>::max(),
        '\n');

    std::getline(
        input,
        result.summary);

    return result;
}


static uint32_t highest_degree_vertex(
    const CSRGraph& graph) {

    uint32_t best = 0;

    uint64_t best_degree =
        graph.degree(0);

    for (uint32_t vertex = 1;
         vertex < graph.vertices;
         ++vertex) {

        const uint64_t degree =
            graph.degree(vertex);

        if (degree > best_degree) {
            best_degree = degree;
            best = vertex;
        }
    }

    return best;
}


static void print_result(
    const std::string& algorithm,
    const ExperimentResult& result) {

    std::cout
        << std::left
        << std::setw(24)
        << algorithm
        << " time="
        << std::fixed
        << std::setprecision(3)
        << result.seconds
        << " s"
        << " peak_rss="
        << result.peak_rss
        << " bytes"
        << " ("
        << format_bytes(result.peak_rss)
        << ")"
        << " | "
        << result.summary
        << '\n';
}


int main(int argc, char* argv[]) {

    if (argc != 2) {
        std::cerr
            << "Usage: "
            << argv[0]
            << " <graph.bin>\n";

        return EXIT_FAILURE;
    }

    try {
        const std::string graph_path =
            argv[1];

        CSRGraph graph;

        graph.load(graph_path);

        std::cout
            << "Graph: "
            << graph_path
            << '\n';

        std::cout
            << "Vertices: "
            << graph.vertices
            << '\n';

        std::cout
            << "Edges: "
            << graph.edges
            << '\n';

        std::cout
            << "CSR neighbors: "
            << graph.neighbors.size()
            << '\n';

        const uint32_t source =
            highest_degree_vertex(graph);

        std::cout
            << "Dijkstra source: "
            << source
            << " (degree="
            << graph.degree(source)
            << ")\n";

        std::cout
            << "\nRunning algorithms...\n";

        const ExperimentResult cc =
            run_isolated([&]() {

                const ComponentResult result =
                    connected_components(graph);

                std::ostringstream out;

                out << "components="
                    << result.count
                    << ", largest="
                    << result.largest;

                return out.str();
            });

        print_result(
            "connected_components",
            cc);

        const ExperimentResult cycle =
            run_isolated([&]() {

                const std::vector<uint32_t> result =
                    one_cycle(graph);

                std::ostringstream out;

                out << "cycle_vertices="
                    << result.size();

                if (!result.empty()) {
                    out << ", start="
                        << result.front()
                        << ", end="
                        << result.back();
                }

                return out.str();
            });

        print_result(
            "one_cycle",
            cycle);

        const ExperimentResult shortest =
            run_isolated([&]() {

                const DijkstraResult result =
                    shortest_paths(
                        graph,
                        source);

                std::ostringstream out;

                out << "reachable="
                    << result.reachable
                    << ", max_distance="
                    << result.max_distance;

                return out.str();
            });

        print_result(
            "shortest_paths",
            shortest);

    } catch (const std::exception& e) {

        std::cerr
            << "ERROR: "
            << e.what()
            << '\n';

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
