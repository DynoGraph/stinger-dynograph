#pragma once

#include "args.h"
#include "batch.h"
#include <vector>
#include <string>

namespace DynoGraph {

class DynamicGraph
{
public:
    const Args args;
    // Initialize an empty graph - you must provide a constructor with this signature
    DynamicGraph(Args args, int64_t max_vertex_id) : args(args) {}
    // Initialize a graph from an edge list - you must provide a constructor with this signature
    DynamicGraph(Args args, int64_t max_vertex_id, const Batch& batch) : args(args) {}
    virtual ~DynamicGraph() {}
    // Return list of supported algs - your class must implement this method
    static std::vector<std::string> get_supported_algs();
    // Prepare to insert the batch
    virtual void before_batch(const Batch& batch, int64_t threshold) = 0;
    // Delete edges in the graph with a timestamp older than <threshold>
    virtual void delete_edges_older_than(int64_t threshold) = 0;
    // Insert the batch of edges into the graph
    virtual void insert_batch(const Batch& batch) = 0;
    // Run the specified algorithm
    virtual void update_alg(
            // Name of algorithm to run
            const std::string &alg_name,
            // Source vertex(s), (applies to bfs, sampled bc, sssp, etc.)
            const std::vector<int64_t> &sources,
            // Contains the results from the previous epoch (for incremental algorithms)
            // Results should be written to this array
            Range<int64_t> data
    ) = 0;
    // Return the degree of the specified vertex
    virtual int64_t get_out_degree(int64_t vertex_id) const = 0;
    // Return the number of vertices in the graph
    virtual int64_t get_num_vertices() const = 0;
    // Return the number of unique edges in the graph
    virtual int64_t get_num_edges() const = 0;
    // Return a list of the n vertices with the highest degrees
    virtual std::vector<int64_t> get_high_degree_vertices(int64_t n) const = 0;
};

// Holds a vertex id and its out degree
// May be useful for implementing DynamicGraph::get_high_degree_vertices
struct vertex_degree
{
    int64_t vertex_id;
    int64_t out_degree;
    vertex_degree() {}
    vertex_degree(int64_t vertex_id, int64_t out_degree)
            : vertex_id(vertex_id), out_degree(out_degree) {}
};
inline bool
operator < (const vertex_degree &a, const vertex_degree &b) {
    if (a.out_degree != b.out_degree) { return a.out_degree < b.out_degree; }
    return a.vertex_id > b.vertex_id;
}

#ifdef USE_MPI
BOOST_IS_BITWISE_SERIALIZABLE(DynoGraph::vertex_degree);
#endif

} // end namespace DynoGraph