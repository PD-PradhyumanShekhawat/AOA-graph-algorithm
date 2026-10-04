# AI Usage Record

Generative AI was used as a study and engineering assistant during this assignment. I remain responsible for the final code, measurements, interpretation, source attribution, and submission.

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
11. Planning and reviewing the optional real-world graph bonus.
12. Comparing alternative adjacency definitions for the NASA HIRENASD mesh.
13. Reviewing source attribution and reproducibility documentation.

## Verification

The required implementation was compiled and executed locally. The final correctness test produced:

```text
All graph algorithm tests passed.
```

The required simulator was executed locally. The final raw CSV contains 61 lines: one header plus 60 measured configurations. Measurements in the required report were taken from that CSV and were not fabricated.

For the optional bonus, the NASA HIRENASD source was downloaded and preprocessed locally. Three CSR graph representations were generated and the three graph algorithms were executed on each representation. The measured bonus values in the report and `bonus/nasa/results/bonus_execution.csv` are the values observed during those local runs.

## Refinement

The cycle reconstruction in `one_cycle()` was corrected after correctness testing exposed an invalid reconstruction. The correctness suite was rerun successfully before the final required experiment.

The real-world CSR loader was corrected to match the actual binary header layout: a 32-bit vertex count followed by a 64-bit edge count. CSR structural validation was then added before algorithm execution.

The memory methodology is explicitly described as process-level peak resident set size (RSS), rather than exact algorithm-only allocation.

## Source attribution

The optional real-world graph uses the NASA HIRENASD coarse unstructured mesh. The source and direct resource URLs are documented in `bonus/nasa/SOURCES.md` and in the combined report. The external NASA dataset is not represented as original work.


## Representative Prompt Types

Representative prompts used during the project included:

- Translate the assignment requirements into an implementation checklist.
- Explain DFS connected components and undirected cycle detection.
- Review the correctness of the Dijkstra implementation with unit edge weights.
- Design correctness tests for the required graph operations.
- Design runtime and peak-memory experiments.
- Compare measured results with theoretical complexity.
- Review the NASA HIRENASD preprocessing and three adjacency definitions.

The prompts were iterative and were used for discussion, review, and refinement.
The final implementation, compilation, execution, measurements, and verification
were performed locally.