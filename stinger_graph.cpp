#include "stinger_graph.h"
#include <vector>
#include <iostream>
#include <hooks.h>
#include <iomanip>

extern "C" {
#include <stinger_core/stinger.h>
#include <stinger_core/core_util.h>
}
#include <stinger_net/stinger_alg.h>
#include <stinger_core/stinger_batch_insert.h>

using std::cerr;

using namespace gt::stinger;

// Figure out how many edge blocks we can allocate to fill STINGER_MAX_MEMSIZE
// Assumes we need just enough room for nv vertices and puts the rest into edge blocks
// Basically implements calculate_stinger_size() in reverse
stinger_config_t generate_stinger_config(int64_t nv) {

    // Start with size we will try to fill
    // Scaled by 75% because that's what stinger_new_full does
    uint64_t sz = ((uint64_t)stinger_max_memsize() * 3)/4;

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



#ifdef USE_STINGER_BATCH_INSERT

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
StingerGraph::insert(const DynoGraph::Batch& batch)
{
    std::vector<EdgeAdapter> updates(batch.size());
    OMP("omp parallel for")
    for (size_t i = 0; i < updates.size(); ++i)
    {
        const DynoGraph::Edge &e = *(batch.begin() + i);
        updates[i] = EdgeAdapter(e);
    }
    Hooks &hooks = Hooks::getInstance();
    hooks.traverse_edges(updates.size());
    if (batch.is_directed())
    { stinger_batch_incr_edges<EdgeAdapter>(S, updates.begin(), updates.end()); }
    else
    { stinger_batch_incr_edge_pairs<EdgeAdapter>(S, updates.begin(), updates.end()); }
}

#else

void
StingerGraph::insert(const DynoGraph::Batch& batch)
{
    // Insert the edges in parallel
    const int64_t type = 0;
    const bool directed = batch.is_directed();

    int64_t chunksize = 8192;
    OMP("omp parallel for schedule(dynamic, chunksize)")
    for (auto e = batch.cbegin(); e < batch.cend(); ++e)
    {
        if (directed)
        {
            stinger_incr_edge     (S, type, e->src, e->dst, e->weight, e->timestamp);
        } else { // undirected
            stinger_incr_edge_pair(S, type, e->src, e->dst, e->weight, e->timestamp);
        }
        Hooks::getInstance().traverse_edges(1);
    }
}`
#endif

// Deletes edges that haven't been modified recently
void
StingerGraph::deleteOlderThan(int64_t threshold)
{
    Hooks &hooks = Hooks::getInstance();
    STINGER_RAW_FORALL_EDGES_OF_ALL_TYPES_BEGIN(S)
    {
        hooks.traverse_edges(1);
        if (STINGER_EDGE_TIME_RECENT < threshold) {
            // Record the deletion
            stinger_edge_update u;
            u.source = STINGER_EDGE_SOURCE;
            u.destination = STINGER_EDGE_DEST;
            u.weight = STINGER_EDGE_WEIGHT;
            u.time = STINGER_EDGE_TIME_RECENT;
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
