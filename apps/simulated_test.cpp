#include "Experiment.h"
#include "GraphGenerators.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

void write_csv_result(
    std::ofstream& output,
    const ExperimentResult& result
) {
    output
        << result.graph_type << ","
        << result.algorithm << ","
        << result.vertices << ","
        << result.edges << ","
        << result.runtime_ms << ","
        << result.peak_memory_kb
        << '\n';
}

void write_formatted_result(
    std::ofstream& output,
    const ExperimentResult& result
) {
    output
        << std::left
        << std::setw(13) << result.graph_type
        << std::setw(23) << result.algorithm
        << std::right
        << std::setw(8) << result.vertices
        << std::setw(12) << result.edges
        << std::setw(17)
        << std::fixed
        << std::setprecision(6)
        << result.runtime_ms
        << std::setw(20)
        << result.peak_memory_kb
        << '\n';
}

void run_and_record(
    const Graph& graph,
    const std::string& graph_type,
    const std::string& algorithm,
    int source,
    std::ofstream& csv_output,
    std::ofstream& formatted_output
) {
    const ExperimentResult result =
        run_experiment(
            graph,
            graph_type,
            algorithm,
            source
        );

    // Store machine-readable experimental data.
    write_csv_result(
        csv_output,
        result
    );

    // Store human-readable experimental data.
    write_formatted_result(
        formatted_output,
        result
    );

    // Display the measured result in the terminal.
    std::cout
        << result.graph_type << ","
        << result.algorithm << ","
        << result.vertices << ","
        << result.edges << ","
        << result.runtime_ms << ","
        << result.peak_memory_kb
        << '\n';
}

int main() {
    const std::vector<int> sparse_sizes = {
        50,
        100,
        500,
        1000,
        2000,
        5000,
        10000
    };

    const std::vector<int> complete_sizes = {
        50,
        100,
        500,
        1000,
        2000,
        5000
    };

    const std::string csv_path =
        "results/raw/experiment_results.csv";

    const std::string formatted_path =
        "results/experiment_results.txt";

    std::ofstream csv_output(csv_path);

    if (!csv_output.is_open()) {
        std::cerr
            << "Error: unable to open "
            << csv_path
            << '\n';

        return 1;
    }

    std::ofstream formatted_output(formatted_path);

    if (!formatted_output.is_open()) {
        std::cerr
            << "Error: unable to open "
            << formatted_path
            << '\n';

        return 1;
    }

    // Write the CSV header.
    csv_output
        << "graph_type,algorithm,vertices,edges,"
        << "runtime_ms,peak_memory_kb\n";

    // Write the human-readable table header.
    formatted_output
        << "COT5405 GRAPH ALGORITHM EXPERIMENT RESULTS\n"
        << "==========================================\n\n";

    formatted_output
        << std::left
        << std::setw(13) << "Graph Type"
        << std::setw(23) << "Algorithm"
        << std::right
        << std::setw(8) << "V"
        << std::setw(12) << "E"
        << std::setw(17) << "Runtime (ms)"
        << std::setw(20) << "Peak Memory (KB)"
        << '\n';

    formatted_output
        << std::string(93, '-')
        << '\n';

    // Run experiments on sparse graph families.
    for (int size : sparse_sizes) {

        // Generate and test cycle graph.
        {
            const Graph graph =
                generate_cycle_graph(size);

            run_and_record(
                graph,
                "cycle",
                "connected_components",
                0,
                csv_output,
                formatted_output
            );

            run_and_record(
                graph,
                "cycle",
                "one_cycle",
                0,
                csv_output,
                formatted_output
            );

            run_and_record(
                graph,
                "cycle",
                "shortest_paths",
                0,
                csv_output,
                formatted_output
            );
        }

        // Generate and test empty graph.
        {
            const Graph graph =
                generate_empty_graph(size);

            run_and_record(
                graph,
                "empty",
                "connected_components",
                0,
                csv_output,
                formatted_output
            );

            run_and_record(
                graph,
                "empty",
                "one_cycle",
                0,
                csv_output,
                formatted_output
            );

            run_and_record(
                graph,
                "empty",
                "shortest_paths",
                0,
                csv_output,
                formatted_output
            );
        }
    }

    // Run experiments on dense complete graphs.
    for (int size : complete_sizes) {

        // Generate and test complete graph.
        {
            const Graph graph =
                generate_complete_graph(size);

            run_and_record(
                graph,
                "complete",
                "connected_components",
                0,
                csv_output,
                formatted_output
            );

            run_and_record(
                graph,
                "complete",
                "one_cycle",
                0,
                csv_output,
                formatted_output
            );

            run_and_record(
                graph,
                "complete",
                "shortest_paths",
                0,
                csv_output,
                formatted_output
            );
        }
    }

    csv_output.close();
    formatted_output.close();

    std::cout
        << "\nResults saved to "
        << csv_path
        << '\n';

    std::cout
        << "Formatted results saved to "
        << formatted_path
        << '\n';

    return 0;
}