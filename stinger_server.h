#pragma once

#include <vector>
#include <iostream>
#include <string>
#include <stdint.h>

#include <dynograph_util.h>

#include "stinger_graph.h"
#include "stinger_algorithm.h"

class StingerServer : public DynoGraph::DynamicGraph
{
private:
    StingerGraph graph;
    std::vector<StingerAlgorithm> algs;
    std::vector<stinger_edge_update> recentInsertions;
    std::vector<stinger_edge_update> recentDeletions;
    int64_t max_active_vertex;

    void onGraphChange();
    void recordGraphStats();
public:

    StingerServer(const DynoGraph::Args& args, int64_t max_nv);

    static std::vector<std::string> get_supported_algs();
    void before_batch(const DynoGraph::Batch& batch, const int64_t threshold);
    void insert_batch(const DynoGraph::Batch & b);
    void delete_edges_older_than(int64_t threshold);
    void update_alg(const std::string &name, const std::vector<int64_t> &sources);

    int64_t get_out_degree(int64_t vertex_id) const;
    int64_t get_num_vertices() const;
    int64_t get_num_edges() const ;
    std::vector<int64_t> get_high_degree_vertices(int64_t n) const;

    struct DistributionSummary
    {
        double mean;
        double variance;
        int64_t max;
        double skew;
    };

    struct DegreeStats
    {
        DistributionSummary both, in, out;
    };

    DegreeStats
    compute_degree_distribution(const DynoGraph::Batch& b);
    DegreeStats
    compute_degree_distribution(StingerGraph& g);
    DegreeStats
    compute_degree_distribution(StingerGraph &g, const DynoGraph::Batch &b);
};
