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
#include <dynograph_util.h>
#include <stinger_core/stinger_atomics.h>
#include "stinger_server.h"

using std::cerr;
using std::shared_ptr;
using std::make_shared;
using std::string;
using std::stringstream;
using std::vector;

using namespace gt::stinger;

StingerServer::StingerServer(const DynoGraph::Args& args, int64_t max_vertex_id)
: DynoGraph::DynamicGraph(args, max_vertex_id)
, graph(max_vertex_id + 1)
, max_active_vertex(0)
{
    graph.printSize();

    algs.reserve(args.alg_names.size());
    // Register algorithms to run
    for (string algName : args.alg_names)
    {
        DynoGraph::Logger::get_instance() << "Initializing " << algName << "...\n";
        algs.emplace_back(graph.S, algName);
        algs.back().onInit();
    }

    onGraphChange();
}

vector<string>
StingerServer::get_supported_algs()
{
    return StingerAlgorithm::supported_algs;
}

void
StingerServer::before_batch(const DynoGraph::Batch& batch, int64_t threshold)
{
    // Compute degree distributions
    Hooks &hooks = Hooks::getInstance();
    DegreeStats batch_degree_dist = compute_degree_distribution(batch);
    hooks.set_stat("batch_degree_mean", batch_degree_dist.both.mean);
    hooks.set_stat("batch_degree_max", batch_degree_dist.both.max);
    hooks.set_stat("batch_degree_variance", batch_degree_dist.both.variance);
    hooks.set_stat("batch_degree_skew", batch_degree_dist.both.skew);
    DegreeStats affected_graph_dist = compute_degree_distribution(graph, batch);
    hooks.set_stat("affected_degree_mean", affected_graph_dist.both.mean);
    hooks.set_stat("affected_degree_max", affected_graph_dist.both.max);
    hooks.set_stat("affected_degree_variance", affected_graph_dist.both.variance);
    hooks.set_stat("affected_degree_skew", affected_graph_dist.both.skew);

    assert(batch.is_directed());
    // Store the insertions in the format that the algorithms expect
    int64_t num_insertions = batch.size();
    recentInsertions.resize(num_insertions);
    OMP("omp parallel for")
    for (int i = 0; i < num_insertions; ++i)
    {
        const DynoGraph::Edge &e = *(batch.begin() + i);
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
        alg.onPre();
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
    int64_t num_edges = stinger_edges_up_to(graph.S, nv);
    DegreeStats d = compute_degree_distribution(graph);

    Hooks &hooks = Hooks::getInstance();
    hooks.set_stat("num_active_vertices", num_active_vertices);
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
StingerServer::insert_batch(const DynoGraph::Batch & b)
{
    if (args.sort_mode == DynoGraph::Args::SORT_MODE::SNAPSHOT) {
        graph.insert_using_set_initial_edges(b);
    } else {
#ifdef USE_STINGER_BATCH_INSERT
        graph.insert_using_stinger_batch(b);
#else
        graph.insert_using_parallel_for(b);
#endif
    }
    onGraphChange();
}

void
StingerServer::delete_edges_older_than(int64_t threshold) {
    graph.deleteOlderThan(threshold);
    onGraphChange();
}

void
StingerServer::update_alg(const string& name, const std::vector<int64_t> &sources)
{
    auto alg = std::find_if(algs.begin(), algs.end(),
        [name](const StingerAlgorithm &alg) { return alg.name == name; });
    if (alg == algs.end()) {
        DynoGraph::Logger::get_instance() << "Algorithm " << name << " was never initialized\n";
        exit(-1);
    }
    alg->setSources(sources);
    alg->onPost();
}

int64_t
StingerServer::get_out_degree(int64_t vertex_id) const
{
    return stinger_outdegree_get(graph.S, vertex_id);
}

int64_t
StingerServer::get_num_vertices() const
{
    return max_active_vertex;
}

int64_t
StingerServer::get_num_edges() const
{
    return stinger_edges_up_to(graph.S, max_active_vertex+1);
}

std::vector<int64_t>
StingerServer::get_high_degree_vertices(int64_t n) const
{
    using DynoGraph::vertex_degree;
    int64_t nv = this->get_num_vertices();
    assert(n < nv);

    std::vector<vertex_degree> degrees(nv);
    #pragma omp parallel for
    for (int64_t i = 0; i < nv; ++i) {
        int64_t degree = stinger_outdegree_get(graph.S, i);
        degrees[i] = vertex_degree(i, degree);
    }
    // order by degree descending, vertex_id ascending
    std::sort(degrees.begin(), degrees.end());

    degrees.erase(degrees.begin(), degrees.end() - n);
    std::vector<int64_t> ids(degrees.size());
    std::transform(degrees.begin(), degrees.end(), ids.begin(),
        [](const vertex_degree &d) { return d.vertex_id; });
    return ids;
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

// Computes the degree distribution of the graph
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

// Computes the degree distribution of the batch (treating the edge list as a graph)
StingerServer::DegreeStats
StingerServer::compute_degree_distribution(const DynoGraph::Batch& b)
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

// Computes the degree distribution of the vertices in the graph that will be updated by this batch
StingerServer::DegreeStats
StingerServer::compute_degree_distribution(StingerGraph& g, const DynoGraph::Batch& b)
{
    // Get a list of unique vertex ID's in the batch
    vector<int64_t> batch_vertices(b.size() * 2);
    std::transform(b.begin(), b.end(), batch_vertices.begin(),
        [](const DynoGraph::Edge& e) { return e.src; });
    std::transform(b.begin(), b.end(), batch_vertices.begin() + b.size(),
        [](const DynoGraph::Edge& e) { return e.dst; });
    std::sort(batch_vertices.begin(), batch_vertices.end());
    vector<int64_t> unique_batch_vertices(b.size() * 2);
    auto end = std::unique_copy(batch_vertices.begin(), batch_vertices.end(), unique_batch_vertices.begin());
    unique_batch_vertices.erase(end, unique_batch_vertices.end());

    // Compute degree distribution for only the vertices in the batch
    DegreeStats stats;
    const stinger_t *S = g.S;
    int64_t n = unique_batch_vertices.size();
    const vector<int64_t> &v = unique_batch_vertices;

    stats.both = summarize(n, [S, v](int64_t i) { return stinger_degree_get(S, v[i]); });
    stats.in   = summarize(n, [S, v](int64_t i) { return stinger_indegree_get(S, v[i]); });
    stats.out  = summarize(n, [S, v](int64_t i) { return stinger_outdegree_get(S, v[i]); });
    return stats;
}