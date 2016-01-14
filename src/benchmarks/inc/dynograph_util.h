/*
 * DynoGraph is a benchmark suite for
 *		microarchitectural simulation of streaming graph workloads
 *
 * Copyright (C) 2014  Georgia Tech Research Institute
 * Jason Poovey (jason.poovey@gtri.gatech.edu)
 * David Ediger (david.ediger@gtri.gatech.edu)
 * Eric Hein (eric.hein@gtri.gatech.edu)
 * Tom Conte (tom@conte.us)

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#if !defined(DYNOGRAPH_H_)
#define DYNOGRAPH_H_

#include <stinger.h>
#include <stdbool.h>

struct dynograph_preloaded_edge {
    int64_t src;
    int64_t dst;
    int64_t weight;
    int64_t timestamp;
};

struct dynograph_preloaded_edges {
    int64_t num_edges;
    int64_t num_batches;
    int64_t edges_per_batch;
    int64_t directed;
    struct dynograph_preloaded_edge edges[0];
};

void dynograph_message(const char* fmt, ...);
void dynograph_error(const char* fmt, ...);

void dynograph_load_graph (stinger_t * S, const char * name);

struct dynograph_preloaded_edges* dynograph_preload_edges(const char* path, int64_t num_batches);
void dynograph_insert_preloaded_batch(stinger_t * S, struct dynograph_preloaded_edges* edges, int64_t batch_id);
void dynograph_free_preloaded_edges(struct dynograph_preloaded_edges* batches);

int64_t dynograph_get_timestamp_for_window(struct dynograph_preloaded_edges *batches, int64_t batch_id, int64_t window_size);

int dynograph_print_fragmentation_stats (stinger_t * S, int64_t nv);
void dynograph_print_graph_size(stinger_t *S, int64_t nv, int64_t modified_after);

#endif /* __DYNOGRAPH_H_ */
