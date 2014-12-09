# DynoGraph

> (c) 2014 Georgia Tech Research Institute
> Released under GPL v3.0.  See LICENSE for full details

DynoGraph is a suite of benchmarks centered
around streaming graph analytics. With the prevalence of
social media and the need to process real-time network graphs,
streaming dynamic graphs have become a very important application.
Many solutions exist for graph-based analysis including
graph databases, custom representation using
formats such as compressed sparse row (CSR), or in-memory
data structures such as STINGER. GraphBench derives
from STINGER, which employs in-memory storage and has
proven results at processing large graphs in real-time.
GraphBench includes three algorithms run on four real-world
inputs: breadth first search, streaming connected components,
and pagerank centrality. These algorithms
are highly data dependent, have unpredictable access
patterns, and are unique in their microarchitecture behavior compared
to other benchmark suites.

> A paper detailing the microarchitecture characteristics of GraphBench is currently under review and will eventually be released here.

## Usage

To build the workloads simply do 

```
make
```

This will build four workloads, `bfs`, `components`, `pagerank`, and `edge_stream`.  The workloads each expect a single input which is a graph edge list file.  The data files are provided in the `data/` directory.

* `coAuthorsDBLP` - Co-authorship network.  Largest graph in suite
* `cond-mat-2003` - Co-authorship network.
* `PGPgiantcompo` - PGP trust network
* `ny_sandy` - Twitter feed collected during Hurricane Sandy

There are **two versions** of each data file, one includes edge or vertex deletions and the other does not.  In an edge list a deletion is noted by a `-` sign preceding the edge.  If the second value in a deletion is a `0`, then this represents a vertex deletion.  If the second value is non-zero, then it is a single edge deletion.

The first three inputs were collected from the 10th annual DIMACS challenge and repurposed to represent the behavior of dynamic graphs.  Ten percent of the original nodes were duplicated and injected into the permuted edge stream.  At the end of the insertion, these nodes are deleted such that the remaining structure is representative of a streaming graph with insertions and deletions happening simultaneously.

The fourth input is a streaming input taken from Twitter.  Batches of edges stream in, and older edges are rolled off periodically.  There are two streaming *phases* to the ny_sandy edge list.  In the first phase insertions and deletions alternate.  In the second phase, a big burst of insertions is injected into the graph.

The **no deletions** versions of the inputs are the same graphs without any injected deletions, creating a more packed graph structure.

> The no deletions version are provided as reference and are representative of static graph analysis, **not** dynamic graph analysis.

Below is a summary of the graph structures and statistics.

| | Vertices | Edges | Edge Blocks | Empty Edge Blocks | Fragmented Edge Blocks | Edge Block Fill % | Avg. Edges per Block | 
| --- | --- | --- | --- | --- | --- | --- | --- |
| `PGPgiantcompo` | 10680 | 24316 | 15939 | 3660 | 7426 | 21.8 | 3.96 |
| `cond-mat-2003` | 30460 | 120029 | 51525 | 11890 | 30941 | 33.3 | 6.06 |
| `coAuthorsDBLP` | 299067 | 977676 | 475650 | 109124 | 270332 | 29.4 | 5.33 |
| `ny_sandy` | 44062 | 77348 | 72717 | 27947 | 1628 | 7.6 | 1.72 |

We do anticipate releasing scripts that allow generation of more versions of each of these input files with different deletion factors, and eventually there will be scripts provided to convert new data sets as GraphBench inputs.

### Test Harness

An effective benchmark requires a test harness that allows for
separation of concerns. Setup and initialization should be done
efficiently during untimed portions of the code, and the region
of interest should be clearly delineated. For the algorithms
only the graph algorithm is timed and measured. Except for the `edge_stream` workload, loading of
the data into the graph from the input files is untimed and
should not be considered for design decisions. This ingest is
done artificially and is not representative of the patterns that
occur during typical streaming graph ingest.

The `edge_stream` workload should use the `ny_sandy` input with deletions only.  More data inputs will be provided in future updates.

To help delineate the test harness, `bench_start()` and `bench_end()` functions calls have been placed in the appropriate locations.  The body of these functions can be found in `src/lib/hooks.c`.

## Algorithms

### BFS

The breadth first search calculates
the distance of every vertex in the graph from a given
source vertex. Two auxiliary data structures are used: a distance score and a flag indicating that a vertex has been processed,
both per-vertex. Two global queues are employed
to track vertices currently being processed (the current frontier)
and vertices about to be processed (the next frontier). At
each level of the breadth first search, starting from the source,
neighbors of vertices in the current frontier that have not been
visited are enqueued in the next frontier and their distance is
set accordingly. This process repeats until all vertices have
been visited or there are no more vertices in the queue.

The breadth first search algorithm has a vertex-centric access
pattern. The edge blocks of the adjacency list of a vertex
are read in succession. Each vertex is processed only once and
each edge block is accessed only once.

### Connected Components

Connected components establishes
a label for each vertex such that vertices with the
same label are in the same component. Vertices in a component
are connected, whereas vertices in different components
are not connected. The Shiloach-Vishkin algorithm for
connected components begins by labeling each vertex in its
own component. In each iteration, all vertices adopt the smallest
label of their neighbors. When the computation converges,
each vertex will be labeled with the identifier of the smallest
vertex in the connected component.

The connected components algorithm is edge-centric. Every
edge block is accessed during each iteration until convergence.
Edge blocks are accessed via a global edge block list for
maximum parallelism and efficient work distribution.

### PageRank Centrality

PageRank centrality calculates the likelihood of arriving at a vertex via random walks about
the graph. It includes a factor that accounts for random jumping
in the network. Each vertex maintains its current centrality
score and distributes that score evenly to all of its neighbors
in each iteration until the algorithm converges. Convergence
is usually reached around 100 iterations.

The PageRank centrality algorithm employs a power iteration,
vertex-centric access pattern that employs a gather
operation to avoid write-locking. Scores are double precision
floating point and allocated per-vertex. The edge blocks of the
adjacency list of a vertex are read in succession during each
iteration.

## Characterization

> Upon release of the accompanying paper on the GraphBench characteristics.  The paper summary will be provided here.


