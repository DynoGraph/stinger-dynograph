/**
 * dynograph_edge_count.h
 *
 * Defines macros for counting the number of edges traversed by each thread in a graph processing benchmark.
 * For each file that does graph processing,
 * 1. Include this header ( and add this path to includes )
 * 2. Add DYNOGRAPH_EDGE_COUNT_TRAVERSE_EDGE whenever an edge is traversed
 */

#ifndef DYNOGRAPH_EDGE_COUNT_H
#define DYNOGRAPH_EDGE_COUNT_H

#ifdef __cplusplus
extern "C" {
#include <cinttypes>
#include <cstddef>
#else
#include <inttypes.h>
#include <stddef.h>
#endif


// Stores edge counts for each thread
// Do not access directly, instead use macros below
extern uint64_t* dynograph_edge_count_num_traversed_edges;

#if defined(_OPENMP)
#include <omp.h>
#define DYNOGRAPH_EDGE_COUNT_THREAD_ID ((size_t)omp_get_thread_num())
#define DYNOGRAPH_EDGE_COUNT_THREAD_COUNT ((size_t)omp_get_max_threads())
#else // single-threaded
#define DYNOGRAPH_EDGE_COUNT_THREAD_ID 0
#define DYNOGRAPH_EDGE_COUNT_THREAD_COUNT 1
#endif

#ifndef ENABLE_DYNOGRAPH_EDGE_COUNT

// Do nothing when disabled for zero overhead
static inline void
dynograph_edge_count_traverse_edges(int64_t n) {}

#else // defined(ENABLE_DYNOGRAPH_EDGE_COUNT)

static inline void
dynograph_edge_count_traverse_edges(int64_t n)
{
    dynograph_edge_count_num_traversed_edges[DYNOGRAPH_EDGE_COUNT_THREAD_ID] += n;
}

#endif // ENABLE_DYNOGRAPH_EDGE_COUNT

#define DYNOGRAPH_EDGE_COUNT_TRAVERSE_EDGE() \
dynograph_edge_count_traverse_edges(1)

#define DYNOGRAPH_EDGE_COUNT_TRAVERSE_MULTIPLE_EDGES(X) \
dynograph_edge_count_traverse_edges(X)

#ifdef __cplusplus
}
#endif
#endif // DYNOGRAPH_EDGE_COUNT_H
