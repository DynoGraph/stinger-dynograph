#include "stinger_algorithm.h"

#include <vector>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <stdint.h>
#include <cmath>
#include <algorithm>

#include <dynograph_util.h>

extern "C" {
#include <stinger_core/stinger.h>
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

using namespace gt::stinger;

// Implementation of StingerAlgorithm

const vector<string>
StingerAlgorithm::supported_algs = {
    "bc",
    "bfs",
    "cc",
    "clustering",
    "simple_communities",
    "simple_communities_updating",
    "streaming_cc",
    "kcore",
    "pagerank"
};

shared_ptr<IDynamicGraphAlgorithm> createImplementation(string name)
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

StingerAlgorithm::StingerAlgorithm(stinger_t * S, string name) :
        // Save the name
        name(name),
        // Create the implementation
        impl(createImplementation(name)),
        // Zero-initialize the server data
        server_data{},
        // Allocate data for the algorithm
        alg_data(impl->getDataPerVertex() * S->max_nv)
{
    // Initialize the "server" data about this algorithm
    strcpy(server_data.alg_name, impl->getName().c_str());
    server_data.stinger = S;
    server_data.alg_data_per_vertex = impl->getDataPerVertex();
    server_data.alg_data = alg_data.data();
    server_data.enabled = true;
}

void
StingerAlgorithm::observeInsertions(vector<stinger_edge_update> &recentInsertions)
{
    server_data.num_insertions = recentInsertions.size();
    server_data.insertions = recentInsertions.data();
}

void
StingerAlgorithm::observeDeletions(vector<stinger_edge_update> &recentDeletions)
{
    server_data.num_deletions = recentDeletions.size();
    server_data.deletions = recentDeletions.data();
}

void
StingerAlgorithm::observeVertexCount(int64_t nv)
{
    server_data.max_active_vertex = nv;
}

void
StingerAlgorithm::setSources(const vector<int64_t> &sources)
{
    if (auto b = std::dynamic_pointer_cast<BreadthFirstSearch>(impl)) {
        b->setSources(sources);
    } else if (auto b = std::dynamic_pointer_cast<BetweennessCentrality>(impl)) {
        b->setSources(sources);
    }
}

void
StingerAlgorithm::onInit(){ impl->onInit(&server_data); }
void
StingerAlgorithm::onPre(){ impl->onPre(&server_data); }
void
StingerAlgorithm::onPost(){ impl->onPost(&server_data); }

// Helper functions to split strings
// http://stackoverflow.com/a/236803/1877086
static void split(const string &s, char delim, vector<string> &elems) {
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
}
static vector<string> split(const string &s, char delim) {
    vector<string> elems;
    split(s, delim, elems);
    return elems;
}

int64_t *
StingerAlgorithm::get_data_ptr()
{
    // Some algorithms provide multiple result arrays
    // Decide which one we want
    string desc;
    if        (name == "bc") { desc = "bc";
    } else if (name == "bfs") { desc = "level";
    } else if (name == "cc") { desc = "component_label";
    } else if (name == "clustering") { desc = "coeff";
    } else if (name == "simple_communities") { desc = "community_label";
    } else if (name == "simple_communities_updating") { desc = "community_label";
    } else if (name == "streaming_cc") { desc = "component_label";
    } else if (name == "kcore") { desc = "kcore";
    } else if (name == "pagerank") { desc = "pagerank";
    } else {
        cerr << "Algorithm " << name << " not implemented!\n";
        exit(-1);
    }

    // Find the position of the result array we want within alg_data
    // Example: Suppose data description is equal to "ll foo bar", and we want data for "bar"
    // We find the index of "bar" to be 3
    // We skip past the elements of "foo" (max_nv * sizeof(l))
    // Then return the pointer
    int64_t max_nv = server_data.stinger->max_nv;
    auto tokens = split(impl->getDataDescription(), ' ');
    auto pos = std::find(tokens.begin(), tokens.end(), desc);
    if (pos == tokens.end()) { return nullptr; }
    // Index of the character that describes the length of our data
    size_t loc = pos - tokens.begin() - 1;
    string spec = tokens[0];
    // This function expects an array of 64-bit type, can't handle smaller types yet
    assert(spec[loc] == 'l' || spec[loc] == 'd');
    uint8_t * data = alg_data.data();
    for (int i = 0; i < loc; ++i)
    {
        switch (spec[i]) {
            case 'f': { data += (sizeof(float) * max_nv); } break;
            case 'd': { data += (sizeof(double) * max_nv); } break;
            case 'i': { data += (sizeof(int32_t) * max_nv); } break;
            case 'l': { data += (sizeof(int64_t) * max_nv); } break;
            case 'b': { data += (sizeof(uint8_t) * max_nv); } break;
        }
    }
    return reinterpret_cast<int64_t*>(data);
}


void
StingerAlgorithm::setData(const DynoGraph::Range<int64_t> &data)
{
    uint64_t max_nv = server_data.stinger->max_nv;
    int64_t * ptr = get_data_ptr();
    #pragma omp parallel for
    for (uint64_t i = 0; i < max_nv; ++i) {
        ptr[i] = data[i];
    }
}

void
StingerAlgorithm::getData(DynoGraph::Range<int64_t> &data)
{
    uint64_t max_nv = server_data.stinger->max_nv;
    const int64_t * ptr = get_data_ptr();
    #pragma omp parallel for
    for (uint64_t i = 0; i < max_nv; ++i) {
        data[i] = ptr[i];
    }
}
