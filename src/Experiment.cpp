#include "Experiment.h"

#include "GraphAlgorithms.h"

#include <chrono>
#include <cstdlib>
#include <stdexcept>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

struct TrialResult {
    double runtime_ms;
    long peak_memory_kb;
};

void run_algorithm(
    const Graph& graph,
    const std::string& algorithm,
    int source
) {
    // Execute the selected graph algorithm.
    if (algorithm == "connected_components") {
        connected_components(graph);
    } else if (algorithm == "one_cycle") {
        one_cycle(graph);
    } else if (algorithm == "shortest_paths") {
        shortest_paths(graph, source);
    } else {
        throw std::invalid_argument(
            "Unknown graph algorithm."
        );
    }
}

TrialResult run_isolated_trial(
    const Graph& graph,
    const std::string& algorithm,
    int source
) {
    int pipe_fds[2];

    if (pipe(pipe_fds) == -1) {
        throw std::runtime_error(
            "Failed to create pipe."
        );
    }

    const pid_t process_id = fork();

    if (process_id == -1) {
        close(pipe_fds[0]);
        close(pipe_fds[1]);

        throw std::runtime_error(
            "Failed to create child process."
        );
    }

    if (process_id == 0) {
        close(pipe_fds[0]);

        TrialResult result{};

        // Measure only the algorithm execution.
        const auto start =
            std::chrono::steady_clock::now();

        run_algorithm(
            graph,
            algorithm,
            source
        );

        const auto end =
            std::chrono::steady_clock::now();

        result.runtime_ms =
            std::chrono::duration<double, std::milli>(
                end - start
            ).count();

        struct rusage usage {};

        if (getrusage(RUSAGE_SELF, &usage) != 0) {
            close(pipe_fds[1]);
            _exit(EXIT_FAILURE);
        }

        // macOS reports ru_maxrss in bytes.
        result.peak_memory_kb =
            usage.ru_maxrss / 1024;

        const ssize_t bytes_written =
            write(
                pipe_fds[1],
                &result,
                sizeof(result)
            );

        close(pipe_fds[1]);

        if (bytes_written != sizeof(result)) {
            _exit(EXIT_FAILURE);
        }

        _exit(EXIT_SUCCESS);
    }

    close(pipe_fds[1]);

    TrialResult result{};

    const ssize_t bytes_read =
        read(
            pipe_fds[0],
            &result,
            sizeof(result)
        );

    close(pipe_fds[0]);

    int status = 0;

    if (waitpid(process_id, &status, 0) == -1) {
        throw std::runtime_error(
            "Failed to wait for child process."
        );
    }

    if (!WIFEXITED(status) ||
        WEXITSTATUS(status) != EXIT_SUCCESS ||
        bytes_read != sizeof(result)) {
        throw std::runtime_error(
            "Isolated experiment failed."
        );
    }

    return result;
}

} // namespace

ExperimentResult run_experiment(
    const Graph& graph,
    const std::string& graph_type,
    const std::string& algorithm,
    int source,
    int repetitions
) {
    if (repetitions <= 0) {
        throw std::invalid_argument(
            "Number of repetitions must be positive."
        );
    }

    double total_runtime_ms = 0.0;
    long maximum_peak_memory_kb = 0;

    // Run independent trials to obtain stable measurements.
    for (int trial = 0; trial < repetitions; ++trial) {
        const TrialResult result =
            run_isolated_trial(
                graph,
                algorithm,
                source
            );

        total_runtime_ms += result.runtime_ms;

        if (result.peak_memory_kb >
            maximum_peak_memory_kb) {
            maximum_peak_memory_kb =
                result.peak_memory_kb;
        }
    }

    const double average_runtime_ms =
        total_runtime_ms / repetitions;

    return {
        graph_type,
        algorithm,
        graph.vertex_count(),
        graph.edge_count(),
        average_runtime_ms,
        maximum_peak_memory_kb
    };
}