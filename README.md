# COT5405 Analysis of Algorithms - Assignment 1 — Graph Algorithms

This repository contains the C++17 implementation and experimental evaluation for COT5405 Analysis of Algorithms Assignment 1.

## Implementation
The project uses an **undirected graph** represented by an adjacency list and implements:
- `connected_components()` — DFS
- `one_cycle()` — DFS cycle detection
- `shortest_paths()` — Dijkstra with implicit edge weight 1

Three graph generators are included:
- cycle graph
- complete graph
- empty graph

## Correctness
The final correctness test produced:

```text
All graph algorithm tests passed.
```

The test suite covers empty/single-vertex, connected/disconnected, acyclic/cyclic, multiple cycles, multiple shortest paths, unreachable vertices, source-to-source paths, and generator edge counts.

## Experiments
Each configuration was repeated five times. Runtime is averaged and peak resident set size (RSS) is the maximum observed across repetitions.

Final sizes:
- cycle: 50, 100, 500, 1000, 2000, 5000, 10000
- empty: 50, 100, 500, 1000, 2000, 5000, 10000
- complete: 50, 100, 500, 1000, 2000, 5000

The complete graph stops at 5000 vertices because its edge count is quadratic; at 5000 vertices it contains 12,497,500 edges.

The final dataset contains 60 configurations and 300 individual algorithm executions.

## Files
- `results/experiment_results.txt` — formatted measurements
- `results/raw/experiment_results.csv` — raw measurements
- `results/processed/summary_table.csv` — processed summary
- `report/report.tex` — final report source
- `report/report.pdf` — compiled report
- `report/figures/` — report figures

## Memory methodology
Algorithms are executed in isolated child processes. Peak resident set size is measured through process resource usage. This is a process-level peak-memory measurement, not an exact per-algorithm allocation trace.

## AI usage
AI assistance was used for requirements analysis, algorithm discussion, implementation review, test design, experiment design, measured-result interpretation, and report drafting. Local compilation, execution, correctness tests, and the reported measurements were verified by the student. Details are in `docs/ai_usage.md`.

## Submission
The required assignment is completed before any optional real-world-graph bonus work.
