#include <algorithm>
#include <vector>
#include <map>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <stdint.h>
#include <cmath>

#include <hooks.h>
#include <dynograph_util.hh>
#include <stinger_core/stinger_atomics.h>
#include "stinger_server.h"

using std::cerr;
using std::shared_ptr;
using std::make_shared;
using std::string;
using std::stringstream;
using std::vector;

using namespace gt::stinger;
using DynoGraph::msg;

// Helper functions to split strings
// http://stackoverflow.com/a/236803/1877086
void split(const string &s, char delim, vector<string> &elems) {
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
}
vector<string> split(const string &s, char delim) {
    vector<string> elems;
    split(s, delim, elems);
    return elems;
}

StingerServer::StingerServer(int64_t nv, std::string alg_names)
: graph(nv), max_active_vertex(0)
{
    graph.printSize();

    // Register algorithms to run
    for (string algName : split(alg_names, ' '))
    {
        cerr << msg << "Initializing " << algName << "...\n";
        algs.emplace_back(graph.S, algName);
        algs.back().onInit();
    }

    onGraphChange();
}

void
StingerServer::prepare(DynoGraph::Batch& batch, int64_t threshold)
{
    // Store the insertions in the format that the algorithms expect
    assert(batch.dataset.isDirected()); // Not sure what algs expect for undirected batch
    int64_t num_insertions = batch.end() - batch.begin();
    recentInsertions.resize(num_insertions);
    OMP("omp parallel for")
    for (int i = 0; i < num_insertions; ++i)
    {
        DynoGraph::Edge &e = *(batch.begin() + i);
        stinger_edge_update &u = recentInsertions[i];
        u.source = e.src;
        u.destination = e.dst;
        u.weight = e.weight;
        u.time = e.timestamp;
    }

    // Figure out which deletions will actually happen
    recentDeletions.clear();
    // Each thread gets a vector to record deletions
    vector<vector<stinger_edge_update>> myDeletions(omp_get_max_threads());
    // Identical to the deletion loop, but we won't delete anything yet
    STINGER_PARALLEL_FORALL_EDGES_OF_ALL_TYPES_BEGIN(graph.S)
                            {
                                if (STINGER_EDGE_TIME_RECENT < threshold) {
                                    // Record the deletion
                                    stinger_edge_update u;
                                    u.source = STINGER_EDGE_SOURCE;
                                    u.destination = STINGER_EDGE_DEST;
                                    u.weight = STINGER_EDGE_WEIGHT;
                                    u.time = STINGER_EDGE_TIME_RECENT;
                                    myDeletions[omp_get_thread_num()].push_back(u);
                                }
                            }
    STINGER_PARALLEL_FORALL_EDGES_OF_ALL_TYPES_END();

    // Combine each thread's deletions into a single array
    for (int i = 0; i < omp_get_max_threads(); ++i)
    {
        recentDeletions.insert(recentDeletions.end(), myDeletions[i].begin(), myDeletions[i].end());
    }

    // Point all the algorithms to the record of insertions and deletions that will occur
    for (auto &alg : algs)
    {
        alg.observeInsertions(recentInsertions);
        alg.observeDeletions(recentDeletions);
    }
}

void
StingerServer::onGraphChange()
{
    // Count the number of active vertices
    // Lots of algs need this, so we'll do it in the server to save time
    max_active_vertex = stinger_max_active_vertex(graph.S);
    for (auto &alg : algs)
    {
        alg.observeVertexCount(max_active_vertex);
    }
    recordGraphStats();
}

void
StingerServer::recordGraphStats()
{
    int64_t nv = max_active_vertex + 1;
    stinger_fragmentation_t stats;
    stinger_fragmentation (graph.S, nv, &stats);
    int64_t num_active_vertices = stinger_num_active_vertices(graph.S);
    DegreeStats d = compute_degree_distribution(graph);

    Hooks &hooks = Hooks::getInstance();
    hooks.set_stat("num_vertices", nv);
    hooks.set_stat("num_active_vertices", num_active_vertices);
    hooks.set_stat("num_edges", stats.num_edges);
    hooks.set_stat("num_empty_edges", stats.num_empty_edges);
    hooks.set_stat("num_fragmented_blocks", stats.num_fragmented_blocks);
    hooks.set_stat("edge_blocks_in_use", stats.edge_blocks_in_use);
    hooks.set_stat("num_empty_blocks", stats.num_empty_blocks);
    hooks.set_stat("degree_mean", d.both.mean);
    hooks.set_stat("degree_max", d.both.max);
    hooks.set_stat("degree_variance", d.both.variance);
    hooks.set_stat("degree_skew", d.both.skew);
}

void
StingerServer::insert(DynoGraph::Batch & b)
{
    graph.insert(b);
    onGraphChange();
}

void
StingerServer::deleteOlderThan(int64_t threshold) {
    graph.deleteOlderThan(threshold);
    onGraphChange();
}

void
StingerServer::updateAlgorithmsBeforeBatch()
{
    Hooks &hooks = Hooks::getInstance();
    for (auto &alg : algs)
    {
        hooks.region_begin(alg.name() + "_pre");
        alg.onPre();
        hooks.region_end();
    }
}

void
StingerServer::updateAlgorithmsAfterBatch()
{
    Hooks &hooks = Hooks::getInstance();
    for (auto &alg : algs)
    {
        // HACK need to pick source vertices outside of timed section
        alg.pickSources();
        hooks.region_begin(alg.name() + "_post");
        alg.onPost();
        hooks.region_end();
    }
}


/**
 * Computes the mean, variance, max, and skew of the (in/out)degree of all vertices in the graph
 */

template <typename getter>
StingerServer::DistributionSummary
summarize(int64_t n, getter get)
{
    StingerServer::DistributionSummary d = {};

    // First pass: compute mean and max
    int64_t max = 0;
    int64_t mean_sum = 0;
    OMP("omp parallel for reduction(max : max), reduction(+ : mean_sum)")
    for (int64_t v = 0; v < n; ++v)
    {
        int64_t degree = get(v);
        mean_sum += degree;
        if (degree > max) { max = degree; }
    }
    d.mean = static_cast<double>(mean_sum) / n;
    d.max = max;

    // Second pass: compute second and third central moments about the mean (for variance and skewness)
    double x2_sum = 0;
    double x3_sum = 0;
    OMP("omp parallel for reduction(+ : x2_sum, x3_sum)")
    for (int64_t v = 0; v < n; ++v)
    {
        int64_t degree = get(v);
        x2_sum += pow(degree - d.mean, 2);
        x3_sum += pow(degree - d.mean, 3);
    }
    d.variance = x2_sum / n;
    d.skew = x3_sum / pow(d.variance, 1.5);

    return d;
}

StingerServer::DegreeStats
StingerServer::compute_degree_distribution(StingerGraph& g)
{
    int64_t n = max_active_vertex + 1;
    DegreeStats stats;
    const stinger_t *S = g.S;

    stats.both = summarize(n, [S](int64_t i) { return stinger_degree_get(S, i); });
    stats.in   = summarize(n, [S](int64_t i) { return stinger_indegree_get(S, i); });
    stats.out  = summarize(n, [S](int64_t i) { return stinger_outdegree_get(S, i); });
    return stats;
}

StingerServer::DegreeStats
StingerServer::compute_degree_distribution(DynoGraph::Batch& b)
{
    // Calculate the in/out degree of each vertex in the batch
    int64_t max_src = std::max_element(b.begin(), b.end(),
            [](const DynoGraph::Edge& a, const DynoGraph::Edge& b) { return a.src < b.src; }
    )->src;
    int64_t max_dst = std::max_element(b.begin(), b.end(),
            [](const DynoGraph::Edge& a, const DynoGraph::Edge& b) { return a.dst < b.dst; }
    )->dst;
    int64_t n = std::max(max_src, max_dst) + 1;
    vector<int64_t> degree(n);
    vector<int64_t> in_degree(n);
    vector<int64_t> out_degree(n);
    OMP("omp parallel for")
    for (auto e = b.begin(); e < b.end(); ++e)
    {
        stinger_int64_fetch_add(&degree[e->src], 1);
        stinger_int64_fetch_add(&degree[e->dst], 1);
        stinger_int64_fetch_add(&out_degree[e->src], 1);
        stinger_int64_fetch_add(&in_degree[e->dst], 1);
    }

    // Summarize
    DegreeStats stats;
    stats.both = summarize(n, [&degree]    (int64_t i) { return degree[i]; });
    stats.in   = summarize(n, [&in_degree] (int64_t i) { return in_degree[i]; });
    stats.out  = summarize(n, [&out_degree](int64_t i) { return out_degree[i]; });

    return stats;
}
