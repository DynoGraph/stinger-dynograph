#ifndef REFERENCE_IMPL_H
#define REFERENCE_IMPL_H

#include "dynamic_graph.h"
#include <map>
#include <cinttypes>

/**
 * Reference implementation of the DynoGraph::DynamicGraph interface
*/
class reference_impl : public DynoGraph::DynamicGraph {

protected:
    struct edge_prop {
        int64_t weight;
        int64_t timestamp;
        edge_prop() : weight(-1), timestamp(-1) {};
        edge_prop(int64_t weight, int64_t timestamp) : weight(weight), timestamp(timestamp) {};
    };
    typedef std::map<int64_t, edge_prop> edge_list;
    typedef std::map<int64_t, edge_list> adjacency_list;
    adjacency_list graph;
    int64_t num_edges;

public:

    reference_impl(DynoGraph::Args args, int64_t max_vertex_id)
    : DynoGraph::DynamicGraph(args, max_vertex_id), num_edges(0) {};

    static std::vector<std::string> get_supported_algs() { return {}; };
    // Prepare to insert the batch
    virtual void before_batch(const DynoGraph::Batch& batch, int64_t threshold) {};
    // Delete edges in the graph with a timestamp older than <threshold>
    virtual void delete_edges_older_than(int64_t threshold)
    {
        for (adjacency_list::iterator vertex = graph.begin(); vertex != graph.end();)
        {
            edge_list& neighbors = vertex->second;
            for (edge_list::iterator neighbor = neighbors.begin(); neighbor != neighbors.end();)
            {
                int64_t timestamp = neighbor->second.timestamp;
                if (timestamp < threshold) {
                    neighbor = neighbors.erase(neighbor);
                    --num_edges;
                } else {
                    ++neighbor;
                }
            }
            // Remove vertex if all out edges are gone
            if (vertex->second.size() == 0)
            {
                vertex = graph.erase(vertex);
            } else {
                ++vertex;
            }
        }
    }
    // Insert the batch of edges into the graph
    virtual void insert_batch(const DynoGraph::Batch& batch)
    {
        for (DynoGraph::Edge e : batch)
        {
            // Locate edge in graph (will create if does not exist)
            edge_prop& edge = graph[e.src][e.dst];
            if (edge.weight == -1) {
                // Insert new edge
                edge.weight = e.weight;
                edge.timestamp = e.timestamp;
                ++num_edges;
            } else {
                // Update existing edge
                edge.weight += e.weight;
                edge.timestamp = std::max(edge.timestamp, e.timestamp);
            }
        }
    }
    // Run the specified algorithm
    virtual void update_alg(const std::string &alg_name, const std::vector<int64_t> &sources, DynoGraph::Range<int64_t> data) {};
    // Return the degree of the specified vertex
    virtual int64_t get_out_degree(int64_t vertex_id) const
    {
        adjacency_list::const_iterator vertex = graph.find(vertex_id);
        return vertex != graph.end() ? static_cast<int64_t>(vertex->second.size()) : 0;
    }
    // Return the number of vertices in the graph
    virtual int64_t get_num_vertices() const
    {
        return static_cast<int64_t>(graph.size());
    }
    // Return the number of unique edges in the graph
    virtual int64_t get_num_edges() const
    {
        return num_edges;
    }

    virtual std::vector<int64_t> get_high_degree_vertices(int64_t n) const
    {
        // Get a list of the degree of every vertex
        using DynoGraph::vertex_degree;
        std::vector<vertex_degree> degrees(graph.size());
        std::transform(graph.begin(), graph.end(), degrees.begin(),
            [](const std::pair<const int64_t, edge_list>& vertex) {
                return vertex_degree(vertex.first, vertex.second.size());
            }
        );
        // Sort in order of increasing degree
        std::sort(degrees.begin(), degrees.end());
        // Chop off the top n
        degrees.erase(degrees.begin(), degrees.end() - n);
        // Return list of vertex ID's
        std::vector<int64_t> sources(n);
        std::transform(degrees.begin(), degrees.end(), sources.begin(),
            [](const vertex_degree &a) { return a.vertex_id; });
        return sources;
    }

    void dump_edges() const
    {
        for (const std::pair<const int64_t, edge_list>& vertex : graph)
        {
            int64_t source = vertex.first;
            const edge_list& neighbors = vertex.second;
            for (edge_list::const_iterator neighbor = neighbors.begin(); neighbor != neighbors.end(); ++neighbor)
            {
                int64_t dest = neighbor->first;
                int64_t weight = neighbor->second.weight;
                int64_t timestamp = neighbor->second.timestamp;
                std::cerr << source << " " << dest << " " << weight << " " << timestamp << "\n";
            }
        }
    }
};

#endif
