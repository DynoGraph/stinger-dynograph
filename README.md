# DynoGraph

[![Build Status](https://travis-ci.org/DynoGraph/stinger-dynograph.svg?branch=master)](https://travis-ci.org/DynoGraph/stinger-dynograph)


> (c) 2016 Georgia Tech Research Institute.
> Released under GPL v3.0.  See LICENSE for full details.

## About

DynoGraph is a benchmark for dynamic graph analytics. DynoGraph models the task of updating a graph with edge insertions and deletions, in addition to common graph algorithms like betweenness centrality, connected components, and PageRank. Over time the in-memory representation of the graph becomes fragmented, introducing more realistic irregularities into the graph traversal process.

This implementation of DynoGraph uses the graph data structure and algorithms from [STINGER](http://stingergraph.com/).

## Building

1. Check out the code. This repo uses git [submodules](https://git-scm.com/docs/git-submodule), so to get everything you'll need to add the recursive flag:

    ```
    git clone --recursive git@github.com:DynoGraph/stinger-dynograph.git
    ```

2. Configure the project with CMake:

    ```
    cd stinger-dynograph
    mkdir build && cd build
    cmake ..
    ```

3. Build. Replace '4' with the number of threads on your machine:

    ```
    make dynograph -j 4
    ```

## Running

All the benchmarks are contained in a single executable, `dynograph`, which will be placed in the build directory.

```
[DynoGraph] Usage: ./dynograph [OPTIONS]
	--num-epochs	Number of epochs (algorithm updates) in the benchmark
	--input-path	File path to the graph edge list to load (.graph.el or .graph.bin)
	--batch-size	Number of edges in each batch of insertions
	--alg-names	Algorithms to run in each epoch
	--sort-mode	Controls batch pre-processing:
		unsorted (no preprocessing, default),
		presort (sort and deduplicate before insert), or
		snapshot (clear out graph and reconstruct for each batch)
	--window-size	Percentage of the graph to hold in memory (computed using timestamps)
	--num-trials	Number of times to repeat the benchmark
	--help	Print help
```

The DynoGraph test harness incrementally creates a graph from the edge list specified by **input_path**, inserting **batch_size** edges at a time. At several evenly-spaced points in the execution (controlled by **num_epochs**), each of the graph algorithms specified by **alg_names** are run. 

Specifying a value for **window_size** will enable edge deletions for the benchmark. The test harness uses a a sliding time window to determine which edges should be deleted. The size of the window is calculated as a percentage of the difference between the first and last timestamps in the input edge list. For example, if the first edge has a timestamp of 0, and the last edge has a timestamp of 100, then a **window_size** of 0.6 will delete edges that have a timestamp older than t-60, where t is the timestamp of the last inserted edge.   

Specifying a value for **num_trials** will run the same benchmark several times in a row. The input edge list is loaded from disk into memory only once, saving execution time versus repeated invocations of the DynoGraph executable.

### Input format

DynoGraph inputs are formatted as a weighted edge list with timestamps. Each line in the input should be formatted as:

    `src dst weight timestamp`

For example, the line `25 244 6 1447545600` is an edge from vertex 25 to vertex 244, with a weight of 6, created at time 1447545600. There is currently no support for named vertices, so these are raw vertex ID's. Weights need to be positive integers. The timestamp must be represented as an integer. It doesn't matter how timestamps are encoded, as long as the integer representation never decreases from one edge to the next.

Graph inputs in text format should be labeled with a `.graph.el` file extension. DynoGraph also supports a binary graph format, suffixed with `.graph.bin`. The binary format encodes each line as four 64-bit integers; reading this format from disk is much faster because it does not require string parsing.   

### Graph Algorithms

Multiple algorithms may be passed as a quoted, space-separated list. Choices are:

  - `bc` Betweenness Centrality (sampled using the 128 highest-degree vertices)
  - `bfs` Breadth-First Search (from the highest-degree vertex)
  - `cc` Connected Components
  - `streaming_cc` Streaming Connected Components (experimental)
  - `clustering` Clustering Coefficients
  - `simple_communities` Simple Community Detection (experimental)
  - `simple_communities_updating` Streaming Simple Community Detection (experimental)
  - `kcore` K-core detection
  - `pagerank` PageRank

### Edge List Preprocessing

The command line argument **sort_mode** specifies how to sort edge batches before inserting into the graph.

* **unsorted**: Do not sort edge batches before insertion. This is the default that should be used for benchmarking.
* **presort**: Sort and deduplicate edge batches before insertion. Helps performance when there are a lot of duplicate edges.
* **snapshot**: Clear and reload the graph from scratch before each batch, ensuring an unfragmented in-memory layout at the cost of slowdown.

## Hooks

The performance hooks can be configured to interface with several simulators and instrumentation tools. Each region of interest in DynoGraph is surrounded by calls to `region_begin` and `region_end`. By default, these just print the name of the region and the elapsed time. By setting `HOOKS_TYPE` during configuration, these hooks can trigger the start of detailed simulation or performance counter measurement.



