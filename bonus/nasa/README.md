# NASA HIRENASD Real-World Graph Bonus

## Purpose

This bonus applies the three Assignment 1 graph operations to a real-world graph with more than one million vertices and evaluates three different definitions of adjacency.

## Dataset

**NASA HIRENASD coarse unstructured mesh**

NASA Open Data Portal:
https://data.nasa.gov/dataset/hirenasd-coarse-unstructured

NASA DASHlink:
https://c3.ndc.nasa.gov/dashlink/resources/382/

Direct Tecplot ASCII source used:
https://c3.ndc.nasa.gov/dashlink/static/media/dataset/HIRENASD_coarse_nc_volume.dat.gz

NASA describes the coarse mesh as approximately 5.7 million nodes and 14.4 million elements. The preprocessing performed for this project produced 5,663,766 unique global vertices after mapping coincident coordinates across zones.

The source portal does not specify a license. The dataset is therefore explicitly attributed to NASA/DASHlink and is not represented as original data.

## Preprocessing

`preprocessing/build_nasa_graph.cpp` reads the compressed Tecplot ASCII stream, maps zone-local nodes to global vertices using coordinates, constructs edges according to a selected adjacency criterion, removes duplicate undirected edges, and writes a CSR graph.

The CSR format is:

```text
uint32_t vertices
uint64_t edge_count
uint64_t offsets[vertices + 1]
uint32_t neighbors[2 * edge_count]
```

The compressed NASA dataset and generated `.bin` graphs are intentionally excluded from the final archive because of their size. They can be regenerated from the documented source.

## Adjacency criteria

### C1 — Same-element adjacency
Every pair of distinct physical nodes belonging to the same volume element is connected.

### C2 — Mesh-edge adjacency
Only physical element edges are connected. Tetrahedra, pyramids, and prisms use their physical edge sets rather than all node pairs.

### C3 — Boundary-face adjacency
Nodes sharing the same boundary triangle or quadrilateral are connected.

## Measured preprocessing

| Criterion | Vertices | Raw edges | Unique edges | Time | Peak RSS |
|---|---:|---:|---:|---:|---:|
| C1 | 5,663,766 | 171,626,243 | 53,425,834 | 53.95 s | ~3.82 GiB |
| C2 | 5,663,766 | 114,700,195 | 24,939,650 | 50.11 s | ~2.20 GiB |
| C3 | 5,663,766 | 1,022,862 | 534,591 | 38.65 s | ~522 MiB |

## Measured algorithm execution

| Criterion | Algorithm | Time | Peak RSS | Result |
|---|---|---:|---:|---|
| C1 | connected_components | 0.345 s | 0.494 GiB | 1 component; largest 5,663,766 |
| C1 | one_cycle | 0.004 s | 0.049 GiB | 4 cycle vertices; start=end |
| C1 | shortest_paths | 1.251 s | 0.491 GiB | 5,663,766 reachable; max distance 65 |
| C2 | connected_components | 0.222 s | 0.264 GiB | 1 component; largest 5,663,766 |
| C2 | one_cycle | 0.004 s | 0.049 GiB | 4 cycle vertices; start=end |
| C2 | shortest_paths | 1.206 s | 0.277 GiB | 5,663,766 reachable; max distance 75 |
| C3 | connected_components | 0.034 s | 0.053 GiB | 5,493,288 components; largest 170,479 |
| C3 | one_cycle | 0.004 s | 0.049 GiB | 4 cycle vertices; start=end |
| C3 | shortest_paths | 0.037 s | 0.090 GiB | 170,479 reachable; max distance 177 |

Dijkstra source selection is deterministic: the execution program selects a highest-degree vertex and reports its ID and degree. The C1 source was 294859 (degree 32), C2 source was 162250 (degree 27), and C3 source was 296007 (degree 10).

## Validation evidence

The CSR loader validates header sizes, offset-array length and monotonicity, final offset, neighbor count, and neighbor ranges before execution. The measured outputs were also checked for internally consistent component coverage, reachable-vertex counts, and closed cycle representations.

The original assignment correctness suite independently produced:

```text
All graph algorithm tests passed.
```
