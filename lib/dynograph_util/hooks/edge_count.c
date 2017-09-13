#include "edge_count.h"
#include <assert.h>

// Stores edge counts for each thread
uint64_t* dynograph_edge_count_num_traversed_edges;

// Must be called in main thread before any uses of TRAVERSE_EDGE or TRAVERSE_MULTIPLE_EDGES
void
dynograph_edge_count_init()
{
    assert(dynograph_edge_count_num_traversed_edges == NULL);
    dynograph_edge_count_num_traversed_edges =
        (uint64_t*)calloc(DYNOGRAPH_EDGE_COUNT_THREAD_COUNT, sizeof(uint64_t));
}

// Can be used to check whether init has been called
bool
dynograph_edge_count_initialized()
{
    return dynograph_edge_count_num_traversed_edges != NULL;
}

// Resets the edge counter for all threads to 0
void
dynograph_edge_count_reset()
{
    for (size_t i = 0; i < DYNOGRAPH_EDGE_COUNT_THREAD_COUNT; ++i) {
        dynograph_edge_count_num_traversed_edges[i] = 0;
    }
}

// Gets the edge count value for thread i
uint64_t
dynograph_edge_count_get(size_t i)
{
    assert(i < DYNOGRAPH_EDGE_COUNT_THREAD_COUNT);
    return dynograph_edge_count_num_traversed_edges[i];
}

// Must be called after all uses of TRAVERSE_EDGE or TRAVERSE_MULTIPLE_EDGES
void
dynograph_edge_count_free() {
    if (dynograph_edge_count_num_traversed_edges != NULL)
    {
        free(dynograph_edge_count_num_traversed_edges);
        dynograph_edge_count_num_traversed_edges = NULL;
    }
}
