#ifndef EDGE_COUNT_H
#define EDGE_COUNT_H

#include "dynograph_edge_count.h"
#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
// Must be called in main thread before any uses of TRAVERSE_EDGE or TRAVERSE_MULTIPLE_EDGES
void
dynograph_edge_count_init();

// Can be used to check whether init has been called
bool
dynograph_edge_count_initialized();

// Resets the edge counter for all threads to 0
void
dynograph_edge_count_reset();

// Gets the edge count value for thread i
uint64_t
dynograph_edge_count_get(size_t i);

// Must be called after all uses of TRAVERSE_EDGE or TRAVERSE_MULTIPLE_EDGES
void
dynograph_edge_count_free();

#ifdef __cplusplus
}
#endif
#endif // EDGE_COUNT_H