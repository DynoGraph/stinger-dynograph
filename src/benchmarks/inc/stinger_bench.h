/*
 * GraphBench is a benchmark suite for
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
#if !defined(STINGER_BENCH_H_)
#define STINGER_BENCH_H_

#include <stinger.h>
#include <stdbool.h>

struct preloaded_edge {
    int64_t src;
    int64_t dst;
    int64_t weight;
    int64_t timestamp;
};

struct preloaded_edge_batch {
    int64_t num_edges;
    bool directed;
    struct preloaded_edge edges[0];
};

struct preloaded_edge_batches {
    int64_t num_batches;
    struct preloaded_edge_batch *batches[0];
};


void load_graph (stinger_t * S, const char * name);

struct preloaded_edge_batches* preload_batches(const char* base_path);
void insert_preloaded_batch(stinger_t * S, struct preloaded_edge_batch* batch);
int load_edgelist (stinger_t * S, const char * filename);

int bench_print_fragmentation_stats (stinger_t * S, int64_t nv);

#endif /* __STINGER_BENCH_H_ */
