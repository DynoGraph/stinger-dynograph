# DynoGraph

[![Build Status (master)](https://travis-ci.org/DynoGraph/stinger-dynograph.png?branch=master)](https://travis-ci.org/DynoGraph/stinger-dynograph)

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
Usage: ./dynograph sort_mode alg_name input_path num_batches window_size num_trials
```

* **alg_name**: Algorithm to run between inserted batches.

    Multiple algorithms may be passed as a quoted, space-separated list. Choices are:

  - `bc` Betweenness Centrality (sampled)
  - `cc` Connected Components
  - `clustering` Clustering Coefficients
  - `simple_communities` Simple Community Detection (experimental)
  - `simple_communities_updating` Streaming Simple Community Detection (experimental)
  - `kcore` K-core detection
  - `pagerank` PageRank

* **sort_mode**: How to sort edge batches before inserting into the graph.

    * **unsorted**: Do not sort edge batches before insertion. This is the default that should be used for benchmarking.
    * **presort**: Sort and deduplicate edge batches before insertion. Helps performance when there are a lot of duplicate edges.
    * **snapshot**: Clear and reload the graph from scratch before each batch, ensuring an unfragmented in-memory layout at the cost of slowdown.

* **input_path**: Path to graph input file.

    DynoGraph inputs are formatted as a weighted edge list with timestamps. Each line in the input should be formatted as:

    `src dst weight timestamp`

    For example, the line `25 244 6 1447545600` is an edge from vertex 25 to vertex 244, with a weight of 6, created at time 1447545600. There is currently no support for named vertices, so these are raw vertex ID's. Weights need to be positive integers. The timestamp must be represented as an integer. It doesn't matter how timestamps are encoded, as long as the integer representation is monotonically increasing throughout the edge list.

    Graph inputs in text format should be labeled with a `.graph.el` file extension. DynoGraph also supports a binary graph format, suffixed with `.graph.bin`. The binary format encodes each line as four 64-bit integers; reading this format from disk is much faster because it does not require string parsing.

* **num_batches**: Number of batches to split the input edge list into.

    DynoGraph loads the edge list incrementally in a series of batches, running the graph algorithms after each batch has been inserted. The input edge list is split into this many equally-sized batches; if the edge list does not divide evenly, some edges may be left off the end.

* **window_size**: Size of the sliding timestamp window, in batches.

    DynoGraph implements a sliding timestamp window that will introduce deletions into a dataset. After this many batches have been inserted, the benchmark will delete edges that have not been updated since `n` batches ago. To run DynoGraph without deletions, set this parameter equal to **num_batches**.

* **num_trials**: Number of times to run the entire benchmark

    For convenience, DynoGraph can run the same benchmark several times in a row. The input edge list is loaded from disk into memory only once, saving execution time versus repeated invocations of the DynoGraph executable.

## Hooks

The `hooks` submodule can be configured to interface with several simulators and instrumentation tools. Each region of interest in DynoGraph is surrounded by calls to `region_begin` and `region_end`. By default, these just print the name of the region and the elapsed time. By setting `HOOKS_TYPE` during configuration, these hooks can trigger the start of detailed simulation or performance counter measurement.



