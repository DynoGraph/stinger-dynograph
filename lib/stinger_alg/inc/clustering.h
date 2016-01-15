#ifndef STINGER_CLUSTERING_H_
#define STINGER_CLUSTERING_H_

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"

int compare (const void * a, const void * b);
int64_t count_intersections (stinger_t * S, int64_t a, int64_t b, int64_t * neighbors, int64_t d, int64_t modified_after);
int64_t count_triangles (stinger_t * S, uint64_t v, int64_t modified_after);
void count_all_triangles (stinger_t * S, int64_t * ntri, int64_t modified_after);

#endif
