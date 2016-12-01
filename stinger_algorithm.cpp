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

// Returns a list of the highest N vertices in the graph
vector<int64_t>
StingerAlgorithm::find_high_degree_vertices(int64_t num)
{
    int64_t n = data.max_active_vertex+1;
    typedef std::pair<int64_t, int64_t> vertex_degree;
    vector<vertex_degree> degrees(n);
    OMP("parallel for")
    for (int i = 0; i < n; ++i) {
        degrees[i] = std::make_pair(i, stinger_outdegree_get(data.stinger, i));
    }

    // order by degree descending, vertex_id ascending
    std::sort(degrees.begin(), degrees.end(),
        [](const vertex_degree &a, const vertex_degree &b)
        {
            if (a.second != b.second) { return a.second > b.second; }
            return a.first < b.first;
        }
    );

    degrees.erase(degrees.begin() + num, degrees.end());
    vector<int64_t> ids(degrees.size());
    std::transform(degrees.begin(), degrees.end(), ids.begin(),
        [](const vertex_degree &d) { return d.first; });
    return ids;
}

void
StingerAlgorithm::pickSources()
{
    if (auto b = std::dynamic_pointer_cast<BreadthFirstSearch>(impl))
    {
        int64_t source = find_high_degree_vertices(1)[0];
        b->setSource(source);
    } else if (auto b = std::dynamic_pointer_cast<BetweennessCentrality>(impl)) {
        auto samples = find_high_degree_vertices(128);
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
