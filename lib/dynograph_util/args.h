#pragma once

#include <inttypes.h>
#include <vector>
#include <string>
#include <iostream>

namespace DynoGraph {

struct Args
{
    // Number of epochs in the benchmark
    int64_t num_epochs;
    // File path for edge list to load
    std::string input_path;
    // Number of edges to insert in each batch of insertions
    int64_t batch_size;
    // Algorithms to run after each epoch
    std::vector<std::string> alg_names;
    // Batch sort mode:
    enum class SORT_MODE {
        // Do not pre-sort batches
        UNSORTED,
        // Sort and deduplicate each batch before returning it
        PRESORT,
        // Each batch is a cumulative snapshot of all edges in previous batches
        SNAPSHOT
    } sort_mode;
    // Percentage of the graph to hold in memory
    double window_size;
    // Number of times to repeat the benchmark
    int64_t num_trials;
    // Number of times to repeat each epoch
    int64_t num_alg_trials;
    // File path to the list of source vertices to use for graph algorithms
    std::string sources_path;

    Args() = default;
    std::string validate() const;

    static Args parse(int argc, char **argv);
    static void print_help(std::string argv0);
};

std::ostream& operator <<(std::ostream& os, Args::SORT_MODE sort_mode);
std::ostream& operator <<(std::ostream& os, const Args& args);

} // end namespace DynoGraph
