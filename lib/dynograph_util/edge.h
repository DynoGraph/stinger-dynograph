#pragma once

#include <inttypes.h>
#include <iostream>

namespace DynoGraph {

struct Edge
{
    int64_t src;
    int64_t dst;
    int64_t weight;
    int64_t timestamp;
};

inline bool
operator==(const Edge& a, const Edge& b)
{
    return a.src == b.src
        && a.dst == b.dst
        && a.weight == b.weight
        && a.timestamp == b.timestamp;
}

inline std::ostream&
operator<<(std::ostream &os, const Edge &e) {
    os << e.src << " " << e.dst << " " << e.weight << " " << e.timestamp;
    return os;
}

#ifdef USE_MPI
BOOST_IS_BITWISE_SERIALIZABLE(DynoGraph::Edge)
#endif

} // end namespace DynoGraph