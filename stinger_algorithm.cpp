#include "stinger_algorithm.h"

#include <vector>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <stdint.h>
#include <cmath>
#include <algorithm>

#include <dynograph_util.hh>

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
using DynoGraph::msg;

// Implementation of StingerAlgorithm

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
// Create the implementation
        impl(createImplementation(name)),
        // Zero-initialize the server data
        data{},
        // Allocate data for the algorithm
        alg_data(impl->getDataPerVertex() * S->max_nv)
{
    // Initialize the "server" data about this algorithm
    strcpy(data.alg_name, impl->getName().c_str());
    data.stinger = S;
    data.alg_data_per_vertex = impl->getDataPerVertex();
    data.alg_data = alg_data.data();
    data.enabled = true;
}

void
StingerAlgorithm::observeInsertions(vector<stinger_edge_update> &recentInsertions)
{
    data.num_insertions = recentInsertions.size();
    data.insertions = recentInsertions.data();
}

void
StingerAlgorithm::observeDeletions(vector<stinger_edge_update> &recentDeletions)
{
    data.num_deletions = recentDeletions.size();
    data.deletions = recentDeletions.data();
}

void
StingerAlgorithm::observeVertexCount(int64_t nv)
{
    data.max_active_vertex = nv;
}

void
StingerAlgorithm::pickSources()
{
    stinger_t *S = data.stinger;
    auto get_degree = [S](int64_t i){ return stinger_outdegree_get(S, i); };
    if (auto b = std::dynamic_pointer_cast<BreadthFirstSearch>(impl)) {
        int64_t source = DynoGraph::find_high_degree_vertices(1, data.max_active_vertex, get_degree)[0];
        b->setSource(source);
    } else if (auto b = std::dynamic_pointer_cast<BetweennessCentrality>(impl)) {
        auto samples = DynoGraph::find_high_degree_vertices(128, data.max_active_vertex, get_degree);
        b->setSources(samples);
    }
}

void
StingerAlgorithm::onInit(){ impl->onInit(&data); }
void
StingerAlgorithm::onPre(){ impl->onPre(&data); }
void
StingerAlgorithm::onPost(){ impl->onPost(&data); }
string
StingerAlgorithm::name() const { return string(data.alg_name); }
