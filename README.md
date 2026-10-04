# COT5405 Assignment 1 — Graph Algorithms

Author: Pradhyuman Singh Shekhawat

UFID: 55742970; Email: pr.shekhawat@ufl.edu

This submission contains the complete COT5405 Analysis of Algorithms Assignment 1 implementation, experiments, report, and the optional real-world graph bonus.

## Required assignment

The required assignment uses an **undirected graph** represented by an adjacency list and implements:

- `connected_components()` — DFS
- `one_cycle()` — DFS cycle detection
- `shortest_paths()` — Dijkstra with implicit edge weight 1

Three graph generators are included:

- cycle graph
- complete graph
- empty graph

The correctness suite covers empty and single-vertex graphs, connected and disconnected graphs, acyclic and cyclic graphs, multiple cycles, multiple shortest paths, unreachable vertices, source-to-source paths, generator edge counts, and cycle validity.

The final correctness execution produced:

```text
All graph algorithm tests passed.
```

## Required experiments

The simulated experiment contains 60 graph configurations and 300 individual algorithm executions (60 configurations × 3 algorithms × 5 repetitions).

- cycle: 50, 100, 500, 1000, 2000, 5000, 10000 vertices
- empty: 50, 100, 500, 1000, 2000, 5000, 10000 vertices
- complete: 50, 100, 500, 1000, 2000, 5000 vertices

Runtime is averaged over five repetitions. Peak memory is the maximum process-level peak RSS observed over those repetitions. Graph construction is outside the timed algorithm execution.

## Optional real-world graph bonus

The bonus uses the NASA HIRENASD coarse unstructured mesh. The source dataset is approximately 5.7 million nodes and 14.4 million elements according to NASA. After coordinate-based global-node deduplication, the graph contains **5,663,766 unique global vertices**.

Three adjacency criteria are evaluated:

1. **Same-element adjacency:** nodes occurring in the same volume element are pairwise adjacent.
2. **Mesh-edge adjacency:** only physical edges of volume elements are used.
3. **Boundary-face adjacency:** nodes occurring on the same boundary face are pairwise adjacent.

All three criteria use the same global vertex set but produce different edge counts and connectivity. The required graph algorithms are run for all three criteria, with runtime and peak RSS recorded.

### NASA source attribution

NASA Open Data Portal:
https://data.nasa.gov/dataset/hirenasd-coarse-unstructured

NASA DASHlink resource:
https://c3.ndc.nasa.gov/dashlink/resources/382/

Direct Tecplot ASCII resource used:
https://c3.ndc.nasa.gov/dashlink/static/media/dataset/HIRENASD_coarse_nc_volume.dat.gz

The NASA Open Data Portal does not specify a license for this dataset. The dataset is therefore explicitly attributed to NASA/DASHlink. The preprocessing code, graph definitions, execution code, measurements, analysis, and report presented in this project were developed as part of this work using the referenced source data.

## Bonus reproducibility

The original compressed NASA dataset and generated CSR graph binaries are not included because of their size. `bonus/nasa/README.md` documents how to obtain the source and regenerate the graphs.

The bonus source entry points requested by the assignment are:

- `realgraph make.cpp`
- `run realgraph make.cpp`

The preprocessing and execution implementations are also retained under `bonus/nasa/preprocessing/` and `bonus/nasa/execution/`.

## Memory methodology

For both required and bonus experiments, peak memory refers to process-level peak resident set size (RSS). It is not an exact per-algorithm allocation trace.

## AI usage

AI assistance was used for requirements analysis, algorithm discussion, implementation review, test design, experiment design, interpretation of measured results, and report drafting. Local compilation, execution, correctness tests, and reported measurements were verified by the student. Details are documented in `docs/ai_usage.md`.
