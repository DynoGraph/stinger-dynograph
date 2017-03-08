#include "stinger_graph.h"
#include <vector>
#include <iostream>
#include <hooks.h>
#include <iomanip>
#include <numeric>

extern "C" {
#include <stinger_core/stinger.h>
#include <stinger_core/core_util.h>
}
#include <stinger_net/stinger_alg.h>
#include <stinger_core/stinger_batch_insert.h>
#include <dynograph_edge_count.h>

using std::cerr;

using namespace gt::stinger;

// Figure out how many edge blocks we can allocate to fill STINGER_MAX_MEMSIZE
// Assumes we need just enough room for nv vertices and puts the rest into edge blocks
// Basically implements calculate_stinger_size() in reverse
stinger_config_t generate_stinger_config(int64_t nv) {

    // Start with size we will try to fill
    // Scaled by 75% because that's what stinger_new_full does
    int64_t sz = ((int64_t)stinger_max_memsize() * 3)/4;

    // Subtract storage for vertices
    sz -= stinger_vertices_size(nv);
    sz -= stinger_physmap_size(nv);

    // Assume just one etype and vtype
    int64_t netypes = 1;
    int64_t nvtypes = 1;
    sz -= stinger_names_size(netypes);
    sz -= stinger_names_size(nvtypes);

    // Leave room for the edge block tracking structures
    sz -= sizeof(stinger_ebpool);
    sz -= sizeof(stinger_etype_array);

    if (sz < 0) {
        DynoGraph::Logger &logger = DynoGraph::Logger::get_instance();
        logger << "Not enough memory to allocate a STINGER data structure with " << nv << " vertices\n";
        logger << "Need at least " << -sz << " more bytes (not counting edge blocks).\n";
        DynoGraph::die();
    }

    // Finally, calculate how much room is left for the edge blocks themselves
    int64_t nebs = sz / (sizeof(stinger_eb) + sizeof(eb_index_t));

    stinger_config_t config = {
            nv,
            nebs,
            netypes,
            nvtypes,
            0, //size_t memory_size;
            0, //uint8_t no_map_none_etype;
            0, //uint8_t no_map_none_vtype;
            1, //uint8_t no_resize;
    };

    return config;
}

StingerGraph::StingerGraph(int64_t nv)
{
    stinger_config_t config = generate_stinger_config(nv);
    S = stinger_new_full(&config);
}

StingerGraph::~StingerGraph() { stinger_free(S); }



struct EdgeAdapter : public DynoGraph::Edge
{
    int64_t result;
    EdgeAdapter(){}
    EdgeAdapter(const DynoGraph::Edge e) : DynoGraph::Edge(e) { result = 0; }
    typedef EdgeAdapter edge;
    static int64_t get_type(const edge &u) { return 0; }
    static void set_type(edge &u, int64_t v) {  }
    static int64_t get_source(const edge &u) { return u.src; }
    static void set_source(edge &u, int64_t v) { u.src = v; }
    static int64_t get_dest(const edge &u) { return u.dst; }
    static void set_dest(edge &u, int64_t v) { u.dst = v; }
    static int64_t get_weight(const edge &u) { return u.weight; }
    static int64_t get_time(const edge &u) { return u.timestamp; }
    static int64_t get_result(const edge& u) { return u.result; }
    static void set_result(edge &u, int64_t v) { u.result = v; }
};

void
StingerGraph::insert_using_stinger_batch(const DynoGraph::Batch& batch)
{
    std::vector<EdgeAdapter> updates(batch.size());
    OMP("omp parallel for")
    for (size_t i = 0; i < updates.size(); ++i)
    {
        const DynoGraph::Edge &e = *(batch.begin() + i);
        updates[i] = EdgeAdapter(e);
    }
    if (batch.is_directed())
    { stinger_batch_incr_edges<EdgeAdapter>(S, updates.begin(), updates.end()); }
    else
    { stinger_batch_incr_edge_pairs<EdgeAdapter>(S, updates.begin(), updates.end()); }
}

// Stores an edge with a direction
struct directed_edge : public DynoGraph::Edge
{
    int64_t dir;
    directed_edge() = default;
    directed_edge(DynoGraph::Edge e, int64_t dir) : DynoGraph::Edge(e), dir(dir) {}
    int64_t directed_source() const {
        return dir & STINGER_EDGE_DIRECTION_OUT ? src : dst;
    }
    bool operator<(const directed_edge& rhs) const {
        if (src != rhs.src) { return src < rhs.src; }
        if (dst != rhs.dst) { return dst < rhs.dst; }
        static_assert(
            STINGER_EDGE_DIRECTION_BOTH > STINGER_EDGE_DIRECTION_OUT
         && STINGER_EDGE_DIRECTION_OUT > STINGER_EDGE_DIRECTION_IN,
            "Edge direction constants changed, sort order is broken"
        );
        return dir > rhs.dir; // Order by direction decending, so BOTH is chosen during dedup
    }
};

std::ostream& operator<<(std::ostream& os, const directed_edge& e)
{
    const DynoGraph::Edge& base = e;
    os << base;
    switch (e.dir)
    {
        case STINGER_EDGE_DIRECTION_OUT:  os << "  OUT"; break;
        case STINGER_EDGE_DIRECTION_IN:   os << "   IN"; break;
        case STINGER_EDGE_DIRECTION_BOTH: os << " BOTH"; break;
        default:                          os << " XXXX"; break;
    }
    return os;
}

void
StingerGraph::insert_using_set_initial_edges(const DynoGraph::Batch& batch)
{
    using std::vector;

    // Assert no duplicates exist in edge list
    assert(std::adjacent_find(batch.begin(), batch.end(),
        [](const DynoGraph::Edge &a, const DynoGraph::Edge &b) { return a.src == b.src && a.dst == b.dst; }
    ) == batch.end());
    assert(batch.size() > 0);

    // Find the largest vertex ID
    int64_t nv = batch.max_vertex_id() + 1;

    /**
     * stinger stores every edge in two places: an out-edge at the source vertex, and
     * an in-edge at the destination vertex. Additionally, if a given vertex has both
     * an in-edge and an out-edge for the same target, they are combined into a single slot
     *
     * stinger_set_initial_edges() was written before edge directionality was added to stinger,
     * so it won't handle this problem for us. We need to create an edge list that has edges
     * in both directions, associated with the proper vertex, and merged where possible.
     * Then we pass an aligned array containing the direction for each edge
     */

    // Create the list of directed edges
    vector<directed_edge> merged_edges(batch.size() * 2);
    {
        // Split each edge into an out-edge and an in-edge
        vector<directed_edge> directed_edges(batch.size() * 2);
        std::transform(batch.begin(), batch.end(), directed_edges.begin(),
            [](const DynoGraph::Edge &e) { return directed_edge(e, STINGER_EDGE_DIRECTION_OUT); } );
        std::transform(batch.begin(), batch.end(), directed_edges.begin() + batch.size(),
            [](const DynoGraph::Edge &e) {
                // Swap src and dest, otherwise keep the same values
                DynoGraph::Edge r;
                r.src = e.dst;
                r.dst = e.src;
                r.weight = e.weight;
                r.timestamp = e.timestamp;
                return directed_edge(r, STINGER_EDGE_DIRECTION_IN);
            }
        );
        // Sort so edges with the same source end up together
        std::sort(directed_edges.begin(), directed_edges.end());
        // Merge adjacent in/out edges into a bidirectional edge which only takes one slot
        std::partial_sum(directed_edges.begin(), directed_edges.end(), directed_edges.begin(),
            [](const directed_edge& a, const directed_edge& b) {
                directed_edge result = b;
                if (a.src == b.src && a.dst == b.dst) {
                    result.dir = STINGER_EDGE_DIRECTION_BOTH;
                }
                return result;
            }
        );
        // Remove duplicates, leaving behind only merged edges
        std::sort(directed_edges.begin(), directed_edges.end());
        auto end = std::unique_copy(directed_edges.begin(), directed_edges.end(), merged_edges.begin(),
            [](const directed_edge& a, const directed_edge& b) {
                return a.src == b.src && a.dst == b.dst;
            }
        );
        merged_edges.erase(end, merged_edges.end());
    }
    size_t num_edges = merged_edges.size();

    // Compute the offsets into the edge list for each vertex
    vector<int64_t> offsets(nv+1);
    {
        // Store degree of each vertex
        vector<int64_t> degrees(nv);

        OMP("omp parallel for")
        for(int64_t v = 0; v < nv; ++v) {
            // Find the range of edges that have src == v using binary search
            directed_edge key; key.src = v;
            auto range = std::equal_range(merged_edges.begin(), merged_edges.end(), key,
                [](const directed_edge &a, const directed_edge &b) {
                    return a.src < b.src;
                }
            );
            degrees[v] = range.second - range.first;
        }
        // Compute prefix sum on degrees, giving the offset into the edge list for each vertex
        offsets[0] = 0;
        std::partial_sum(degrees.begin(), degrees.end(), offsets.begin()+1);
    } // Extra scope to destroy intermediate lists early

    // Split the edge list into targets and directions
    vector<int64_t> adj_list(num_edges);
    std::transform(merged_edges.begin(), merged_edges.end(), adj_list.begin(),
        [](const directed_edge &e) { return e.dst; } );

    vector<int64_t> directions_list(num_edges);
    std::transform(merged_edges.begin(), merged_edges.end(), directions_list.begin(),
        [](const directed_edge &e) { return e.dir; } );

    // Create the list of weights
    vector<int64_t> weights_list(num_edges);
    std::transform(merged_edges.begin(), merged_edges.end(), weights_list.begin(),
        [](const directed_edge &e) { return e.weight; } );

    // Populate the list of timestamps
    vector<int64_t> ts_list(num_edges);
    std::transform(merged_edges.begin(), merged_edges.end(), ts_list.begin(),
        [](const directed_edge &e) { return e.timestamp; } );

    // Finally, initialize the stinger graph from the CSR representation
    const int64_t etype = 0;
    const int64_t single_ts = 0;
    stinger_set_initial_edges(
        S,
        nv,
        etype,
        offsets.data(),
        adj_list.data(),
        directions_list.data(),
        weights_list.data(),
        ts_list.data(),
        ts_list.data(),
        single_ts
    );

    assert(stinger_consistency_check(S, nv) == 0);
}

void
StingerGraph::insert_using_parallel_for_dynamic_schedule(const DynoGraph::Batch& batch)
{
    // Insert the edges in parallel
    const int64_t type = 0;
    const bool directed = batch.is_directed();

    // Some inserts take longer than others, depending on vertex degree
    // Too many chunks per thread -> synchronization overhead at the work queue
    // Too few chunks per thread -> load imbalance (waiting for a few slow threads)
    int64_t chunks_per_thread = 8;
    int64_t chunksize = std::max(1UL, batch.size() / (omp_get_max_threads()*chunks_per_thread));
    OMP("omp parallel for schedule(dynamic, chunksize)")
    for (auto e = batch.begin(); e < batch.end(); ++e)
    {
        if (directed)
        {
            stinger_incr_edge     (S, type, e->src, e->dst, e->weight, e->timestamp);
        } else { // undirected
            stinger_incr_edge_pair(S, type, e->src, e->dst, e->weight, e->timestamp);
        }
    }
}

void
StingerGraph::insert_using_parallel_for_static_schedule(const DynoGraph::Batch& batch)
{
    // Insert the edges in parallel
    const int64_t type = 0;
    const bool directed = batch.is_directed();

    OMP("omp parallel for schedule(static)")
    for (auto e = batch.begin(); e < batch.end(); ++e)
    {
        if (directed)
        {
            stinger_incr_edge     (S, type, e->src, e->dst, e->weight, e->timestamp);
        } else { // undirected
            stinger_incr_edge_pair(S, type, e->src, e->dst, e->weight, e->timestamp);
        }
    }
}

// Deletes edges that haven't been modified recently
void
StingerGraph::deleteOlderThan(int64_t threshold)
{
    STINGER_RAW_FORALL_EDGES_OF_ALL_TYPES_BEGIN(S)
    {
        DYNOGRAPH_EDGE_COUNT_TRAVERSE_EDGE();
        if (STINGER_EDGE_TIME_RECENT < threshold) {
            // Delete the edge
            update_edge_data_and_direction (S, current_eb__, i__, ~STINGER_EDGE_DEST, 0, 0, STINGER_EDGE_DIRECTION, EDGE_WEIGHT_SET);
        }
    }
    STINGER_RAW_FORALL_EDGES_OF_ALL_TYPES_END();
}

void
StingerGraph::printSize()
{
    size_t stinger_bytes = calculate_stinger_size(S->max_nv, S->max_neblocks, S->max_netypes, S->max_nvtypes).size;
    DynoGraph::Logger &logger = DynoGraph::Logger::get_instance();
    logger << "Initialized stinger with storage for "
         << S->max_nv << " vertices and "
         << S->max_neblocks * STINGER_EDGEBLOCKSIZE << " edges.\n";
    logger << std::setprecision(4);
    logger << "Stinger is consuming " << (double)stinger_bytes / (1024*1024*1024) << "GB of RAM\n";
}
