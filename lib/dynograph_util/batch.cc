#include "batch.h"

using namespace DynoGraph;

int64_t
Batch::num_vertices_affected() const
{
    // Get a list of just the vertex ID's in this batch
    pvector<int64_t> vertices(size() * 2);
    std::transform(begin_iter, end_iter, vertices.begin(),
        [](const Edge& e){ return e.src; });
    std::transform(begin_iter, end_iter, vertices.begin() + size(),
        [](const Edge& e){ return e.dst; });

    // Deduplicate
    pvector<int64_t> unique_vertices(vertices.size());
    std::sort(vertices.begin(), vertices.end());
    auto end = std::unique_copy(vertices.begin(), vertices.end(), unique_vertices.begin());
    return static_cast<int64_t>(end - unique_vertices.begin());
}

int64_t
Batch::max_vertex_id() const {
    auto max_edge = std::max_element(begin_iter, end_iter,
        [](const Edge& a, const Edge& b) {
            return std::max(a.src, a.dst) < std::max(b.src, b.dst);
        }
    );
    return std::max(max_edge->src, max_edge->dst);
}

void
Batch::filter(int64_t threshold)
{
    Edge key = {0, 0, 0, threshold};
    begin_iter = std::lower_bound(begin_iter, end_iter, key,
        [](const Edge& a, const Edge& b) { return a.timestamp < b.timestamp; }
    );
}

void
Batch::dedup_and_sort_by_out_degree()
{
    // Sort to prepare for deduplication
    auto by_src_dest_time = [](const Edge& a, const Edge& b) {
        // Order by src ascending, then dest ascending, then timestamp descending
        // This way the edge with the most recent timestamp will be picked when deduplicating
        return (a.src != b.src) ? a.src < b.src
             : (a.dst != b.dst) ? a.dst < b.dst
             :  a.timestamp > b.timestamp;
    };
    std::sort(begin_iter, end_iter, by_src_dest_time);

    // Deduplicate the edge list
    {
        pvector<Edge> deduped_edges(size());
        // Using std::unique_copy since there is no parallel version of std::unique
        auto end = std::unique_copy(begin_iter, end_iter, deduped_edges.begin(),
            // We consider only source and dest when searching for duplicates
            // The input is sorted, so we'll only get the most recent timestamp
            // BUG: Does not combine weights
            [](const Edge& a, const Edge& b) { return a.src == b.src && a.dst == b.dst; });
        // Copy deduplicated edges back into this batch
        std::transform(deduped_edges.begin(), end, begin_iter,
            [](const Edge& e) { return e; });
        // Adjust size
        size_t num_deduped_edges = (end - deduped_edges.begin());
        end_iter = begin_iter + num_deduped_edges;
    }

    // Allocate an array with an entry for each vertex
    pvector<int64_t> degrees(this->max_vertex_id()+1);
    #pragma omp parallel for
    for (int64_t i = 0; i < degrees.size(); ++i) {
        degrees[i] = i;
    }

    // Count the degree of each vertex
    auto pos = begin_iter;
    #pragma omp parallel for schedule(static) firstprivate(pos)
    for (size_t src = 0; src < degrees.size(); ++src)
    {
        // Find the range of edges with src==src
        Edge key = {src, 0, 0, 0};
        auto range = std::equal_range(pos, end_iter, key,
            [](const Edge& a, const Edge& b) {
                return a.src < b.src;
            }
        );
        // Calculate length of range
        // Since there are no duplicates, this is the degree of src
        degrees[src] = range.second - range.first;
        // Reuse end of range as start of next search
        pos = range.second;
    }

    // Sort by out degree descending, src then dst
    auto by_out_degree = [&degrees](const Edge& a, const Edge& b) {
        if (degrees[a.src] != degrees[b.src]) {
            return degrees[a.src] > degrees[b.src];
        } else {
            return degrees[a.dst] > degrees[b.dst];
        }
    };
    std::sort(begin_iter, end_iter, by_out_degree);
}
