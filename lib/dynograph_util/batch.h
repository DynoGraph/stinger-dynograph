#pragma once

#include "edge.h"
#include "range.h"
#include "pvector.h"
#include <cinttypes>

namespace DynoGraph {

// Represents a list of edges that should be inserted into the graph
class Batch : public Range<Edge>
{

public:
    using Range<Edge>::Range;
    Batch() = default;
    int64_t num_vertices_affected() const;
    int64_t max_vertex_id() const;
    void filter(int64_t threshold);
    void dedup_and_sort_by_out_degree();

    bool is_directed() const { return true; }
    virtual ~Batch() = default;
};

inline std::ostream&
operator<<(std::ostream &os, const Batch &b)
{
    if (b.size() < 20) {
        for (const Edge& e : b) {
            os << e << "\n";
        }
    } else {
        os << "<Batch of " << b.size() << " edges>";
    }
    return os;
}

// Batch subclass with internal storage
class ConcreteBatch : public Batch
{
protected:
    pvector<Edge> edges;
    ConcreteBatch(size_t n) : Batch(), edges(n) {};
public:
    explicit ConcreteBatch(const Batch& batch)
    : Batch(batch)
    // Make a copy of the original batch
    , edges(batch.begin(), batch.end())
    {
        // Reinitialize the batch pointers to point to the owned copy
        begin_iter = &*edges.begin();
        end_iter = &*edges.end();
    }
};

} // end namespace DynoGraph