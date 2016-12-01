#pragma once

#include <vector>
#include <iostream>
#include <string>
#include <stdint.h>

#include <hooks.h>
#include <dynograph_util.hh>

#include "stinger_graph.h"
#include "stinger_algorithm.h"

class StingerServer
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

    StingerServer(int64_t nv, std::string alg_names);

    void prepare(DynoGraph::Batch& batch, int64_t threshold);
    void insert(DynoGraph::Batch & b);
    void deleteOlderThan(int64_t threshold);

    void updateAlgorithmsBeforeBatch();
    void updateAlgorithmsAfterBatch();

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
    compute_degree_distribution(DynoGraph::Batch& b);
    DegreeStats
    compute_degree_distribution(StingerGraph& g);
};
