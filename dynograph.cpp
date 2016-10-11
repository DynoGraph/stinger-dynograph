/*
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <vector>
#include <map>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <stdint.h>

#include <hooks.h>
#include <dynograph_util.hh>

extern "C" {
#include <stinger_core/stinger.h>
#include <stinger_core/core_util.h>
#include <stinger_alg/bfs.h>
#include <stinger_alg/betweenness.h>
#include <stinger_alg/clustering.h>
#include <stinger_alg/static_components.h>
#include <stinger_alg/kcore.h>
#include <stinger_alg/pagerank.h>
}
#include <stinger_alg/streaming_algorithm.h>
#include <stinger_alg/dynamic_betweenness.h>
#include <stinger_alg/dynamic_bfs.h>
#include <stinger_alg/dynamic_static_components.h>
#include <stinger_alg/dynamic_clustering.h>
#include <stinger_alg/dynamic_kcore.h>
#include <stinger_alg/dynamic_pagerank.h>
#include <stinger_alg/dynamic_simple_communities.h>
#include <stinger_alg/dynamic_simple_communities_updating.h>
#include <stinger_alg/dynamic_streaming_connected_components.h>
#include <stinger_net/stinger_alg.h>

using std::cerr;
using std::shared_ptr;
using std::make_shared;
using std::string;
using std::stringstream;
using std::vector;
using std::map;

using namespace gt::stinger;

vector<int64_t> bfs_sources = {3, 30, 300, 4, 40, 400};

struct args
{
    const char* alg_name;
    const char* input_path;
    int64_t window_size;
    int64_t num_batches;
    int64_t num_trials;
    int64_t enable_deletions;
};

struct args
get_args(int argc, char **argv)
{
    struct args args;
    if (argc != 6)
    {
        cerr << "Usage: alg_name input_path num_batches window_size num_trials \n";
        exit(-1);
    }

    args.alg_name = argv[1];
    args.input_path = argv[2];
    args.num_batches = atoll(argv[3]);
    args.window_size = atoll(argv[4]);
    args.num_trials = atoll(argv[5]);
    if (args.window_size == args.num_batches)
    {
        args.enable_deletions = 0;
    } else {
        args.enable_deletions = 1;
    }
    if (args.num_batches < 1 || args.window_size < 1 || args.num_trials < 1)
    {
        cerr << "num_batches, window_size, and num_trials must be positive\n";
        exit(-1);
    }

    return args;
}

struct Algorithm
{
    stinger_registered_alg data;
    shared_ptr<IDynamicGraphAlgorithm> impl;
};

shared_ptr<IDynamicGraphAlgorithm> createAlgorithm(string name)
{
    if        (name == "bc") {
        return make_shared<BetweennessCentrality>(128, 0.5, 1);
    } else if (name == "bfs") {
        return make_shared<BreadthFirstSearch>();
    } else if (name == "cc") {
        return make_shared<ConnectedComponents>();
    } else if (name == "clustering") {
        return make_shared<ClusteringCoefficients>();
    } else if (name == "simple_communities") {
        return make_shared<SimpleCommunities>();
    } else if (name == "simple_communities_updating") {
        return make_shared<SimpleCommunitiesUpdating>(false);
    } else if (name == "streaming_cc") {
        return make_shared<StreamingConnectedComponents>();
    } else if (name == "kcore") {
        return make_shared<KCore>();
    } else if (name == "pagerank") {
        return make_shared<PageRank>("", false, true, EPSILON_DEFAULT, DAMPINGFACTOR_DEFAULT, MAXITER_DEFAULT);
    } else {
        cerr << "Algorithm " << name << " not implemented!\n";
        exit(-1);
    }

}

class DummyServer
{
private:
    stinger_t * S;
    map<string, Algorithm> registry;
    vector<stinger_edge_update> recentInsertions;
    vector<stinger_edge_update> recentDeletions;

    // Figure out how many edge blocks we can allocate to fill STINGER_MAX_MEMSIZE
    // Assumes we need just enough room for nv vertices and puts the rest into edge blocks
    // Basically implements calculate_stinger_size() in reverse
    stinger_config_t generate_stinger_config(int64_t nv) {

        // Start with size we will try to fill
        uint64_t sz = stinger_max_memsize();

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

public:

    DummyServer(uint64_t nv)
    {
        stinger_config_t config = generate_stinger_config(nv);
        S = stinger_new_full(&config);
        printStingerSize();
    }

    ~DummyServer()
    {
        stinger_free(S);
    }

    void printStingerSize()
    {
        size_t stinger_bytes = calculate_stinger_size(S->max_nv, S->max_neblocks, S->max_netypes, S->max_nvtypes).size;
        cerr << DynoGraph::msg <<
        "Initialized stinger with storage for "
        << S->max_nv << " vertices and "
        << S->max_neblocks * STINGER_EDGEBLOCKSIZE << " edges.\n";
        cerr << DynoGraph::msg <<
        "Stinger is consuming " << stinger_bytes / (1024*1024) << "MB of RAM\n";
    }

    void
    registerAlg(string name)
    {
        // Create an entry in the registry
        Algorithm &newAlgorithm = registry[name];

        // Create the implementation
        shared_ptr<IDynamicGraphAlgorithm> impl = createAlgorithm(name);
        newAlgorithm.impl = impl;

        // Initialize the "server" data about this algorithm
        stinger_registered_alg &data = newAlgorithm.data;
        memset(&data, 0, sizeof(data));
        strcpy(data.alg_name, impl->getName().c_str());
        data.stinger = S;
        data.alg_data_per_vertex = impl->getDataPerVertex();
        data.alg_data = malloc(data.alg_data_per_vertex * S->max_nv);
        data.enabled = true;

        impl->onInit(&data);
    }

    void
    prepare(DynoGraph::Batch batch, int64_t threshold)
    {
        // Store the insertions in the format that the algorithms expect
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
        // Identical to the deletion loop below, but we won't delete anything yet
        STINGER_PARALLEL_FORALL_EDGES_OF_ALL_TYPES_BEGIN(S)
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
        for (auto &item : registry)
        {
            stinger_registered_alg &data = item.second.data;
            data.num_insertions = recentInsertions.size();
            data.insertions = recentInsertions.data();
            data.num_deletions = recentDeletions.size();
            data.deletions = recentDeletions.data();
        }
    }

    void
    insert(DynoGraph::Batch batch)
    {
        // Insert the edges in parallel
        const int64_t type = 0;
        const int64_t directed = true; // FIXME
        Hooks::getInstance().region_begin("insertions");
        int64_t chunksize = 8192;
        OMP("omp parallel for schedule(dynamic, chunksize)")
        for (auto e = batch.begin(); e < batch.end(); ++e)
        {
            if (directed)
            {
                stinger_incr_edge     (S, type, e->src, e->dst, e->weight, e->timestamp);
            } else { // undirected
                stinger_incr_edge_pair(S, type, e->src, e->dst, e->weight, e->timestamp);
            }
            Hooks::getInstance().traverse_edge(1);
        }
        Hooks::getInstance().region_end("insertions");

        updateVertexCount();
    }


    // Deletes edges that haven't been modified recently
    void
    deleteOlderThan(int64_t threshold)
    {
        recentDeletions.clear();
        // Each thread gets a vector to record deletions
        vector<vector<stinger_edge_update>> myDeletions(omp_get_max_threads());

        Hooks::getInstance().region_begin("deletions");
        STINGER_RAW_FORALL_EDGES_OF_ALL_TYPES_BEGIN(S)
        {
            if (STINGER_EDGE_TIME_RECENT < threshold) {
                // Record the deletion
                stinger_edge_update u;
                u.source = STINGER_EDGE_SOURCE;
                u.destination = STINGER_EDGE_DEST;
                u.weight = STINGER_EDGE_WEIGHT;
                u.time = STINGER_EDGE_TIME_RECENT;
                myDeletions[omp_get_thread_num()].push_back(u);
                // Delete the edge
                update_edge_data_and_direction (S, current_eb__, i__, ~STINGER_EDGE_DEST, 0, 0, STINGER_EDGE_DIRECTION, EDGE_WEIGHT_SET);
            }
        }
        STINGER_RAW_FORALL_EDGES_OF_ALL_TYPES_END();

        Hooks::getInstance().region_end("deletions");
        // Combine each thread's deletions into a single array
        for (int i = 0; i < omp_get_max_threads(); ++i)
        {
            recentDeletions.insert(recentDeletions.end(), myDeletions[i].begin(), myDeletions[i].end());
        }

        updateVertexCount();
    }

    void
    updateVertexCount()
    {
        // Count the number of active vertices
        // Lots of algs need this, so we'll do it in the server to save time
        int64_t max_active_vertex = stinger_max_active_vertex(S);
        for (auto &item : registry)
        {
            stinger_registered_alg &data = item.second.data;
            data.max_active_vertex = max_active_vertex;
        }
    }

    void
    pickSources(Algorithm& alg)
    {
        if (auto b = std::dynamic_pointer_cast<BreadthFirstSearch>(alg.impl))
        {
            b->pickSource(&alg.data);
        } else if (auto b = std::dynamic_pointer_cast<BetweennessCentrality>(alg.impl)) {
            b->pickSources(&alg.data);
        }
    }

    void
    updateAlgorithmsBeforeBatch()
    {
        for (auto &item : registry)
        {
            string name = item.first;
            Algorithm &alg = item.second;
            Hooks::getInstance().region_begin(name + "_pre");
            alg.impl->onPre(&alg.data);
            Hooks::getInstance().region_end(name + "_pre");
        }
    }

    void
    updateAlgorithmsAfterBatch()
    {
        for (auto &item : registry)
        {
            string name = item.first;
            Algorithm &alg = item.second;
            // HACK need to generate random vertices outside of critical section
            pickSources(alg);
            Hooks::getInstance().region_begin(name + "_post");
            alg.impl->onPost(&alg.data);
            Hooks::getInstance().region_end(name + "_post");
        }
    }

    // Counts the number of edges that satsify the filter
    int64_t
    filtered_edge_count ( int64_t modified_after)
    {
        int64_t num_edges = 0;
        OMP ("omp parallel for reduction(+:num_edges)")
        for (int64_t v = 0; v < S->max_nv; v++) {
            STINGER_FORALL_OUT_EDGES_OF_VTX_MODIFIED_AFTER_BEGIN (S, v, modified_after) {
                num_edges += 1;
            } STINGER_FORALL_OUT_EDGES_OF_VTX_MODIFIED_AFTER_END();
        }
        return num_edges;
    }

    void printGraphStats()
    {
        stinger_fragmentation_t * stats = (stinger_fragmentation_t*)xmalloc(sizeof(struct stinger_fragmentation_t));
        int64_t nv = stinger_max_active_vertex(S) + 1;
        stinger_fragmentation (S, nv, stats);
        printf("{\n");
        printf("\"num_vertices\"            :%ld,\n", nv);
        //printf("\"num_filtered_edges\"      :%ld,\n", filtered_edge_count(S, nv, modified_after));
        printf("\"num_edges\"               :%ld,\n", stats->num_edges);
        printf("\"num_empty_edges\"         :%ld,\n", stats->num_empty_edges);
        printf("\"num_fragmented_blocks\"   :%ld,\n", stats->num_fragmented_blocks);
        printf("\"edge_blocks_in_use\"      :%ld,\n", stats->edge_blocks_in_use);
        printf("\"num_empty_blocks\"        :%ld\n" , stats->num_empty_blocks);
        printf("}\n");
        free(stats);
    }

};

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

int main(int argc, char **argv)
{
    using DynoGraph::msg;

    // Process command line arguments
    struct args args = get_args(argc, argv);
    // Load graph data in from the file in batches
    DynoGraph::Dataset dataset(args.input_path, args.num_batches);
    Hooks& hooks = Hooks::getInstance();

    for (int64_t trial = 0; trial < args.num_trials; trial++)
    {
        hooks.trial = trial;
        // Create the stinger data structure
        DummyServer server(dataset.getMaxNumVertices());
        // Register algorithms to run
        for (string algName : split(args.alg_name, ' '))
        {
            cerr << msg << "Initializing " << algName << "...\n";
            server.registerAlg(algName);
        }

        // Run the algorithm(s) after each inserted batch
        for (int64_t i = 0; i < dataset.getNumBatches(); ++i)
        {
            hooks.batch = i;
            DynoGraph::Batch batch = dataset.getBatch(i);

            int64_t threshold = dataset.getTimestampForWindow(i, args.window_size);
            server.prepare(batch, threshold);

            cerr << msg << "Running algorithms (pre-processing step)\n";
            server.updateAlgorithmsBeforeBatch();

            if (args.enable_deletions)
            {
                cerr << msg << "Deleting edges older than " << threshold << "\n";
                server.deleteOlderThan(threshold);
            }

            cerr << msg << "Inserting batch " << i << "\n";
            server.insert(batch);

            //TODO re-enable filtering at some point cerr << msg << "Filtering on >= " << threshold << "\n";
            cerr << msg << "Running algorithms (post-processing step)\n";
            server.updateAlgorithmsAfterBatch();

            server.printGraphStats();
        }

    }

    return 0;
}
