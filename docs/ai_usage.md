# AI Usage Record

Generative AI was used as a study and engineering assistant during this assignment. The student remains responsible for the final code, measurements, interpretation, and submission.

## Uses
AI assistance was used for:
1. Translating the assignment into an implementation checklist.
2. Discussing the undirected adjacency-list representation.
3. Explaining DFS connected components and undirected cycle detection.
4. Developing and reviewing algorithm pseudocode.
5. Reviewing C++ implementation decisions and edge cases.
6. Designing correctness tests.
7. Designing cycle, complete, and empty graph generators.
8. Designing runtime and peak-memory experiments.
9. Interpreting measured CSV results against theoretical complexity.
10. Drafting and reviewing the technical report.

## Verification
The implementation was compiled and executed locally.

The final correctness test produced:

```text
All graph algorithm tests passed.
```

The final simulator produced 60 measured configurations. The raw CSV contains 61 lines: one header plus 60 configurations. The report measurements were taken from this final experiment run and were not fabricated.

## Refinement
The cycle reconstruction in `one_cycle()` was corrected after the correctness test exposed an invalid cycle reconstruction. The correctness suite was then rerun successfully before the final experiment.

The memory methodology is explicitly described as process-level peak resident set size (RSS), rather than exact algorithm-only allocation.

## Responsibility
The student is responsible for understanding, checking, and submitting the final implementation and report.
