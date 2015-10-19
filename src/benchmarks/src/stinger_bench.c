/*
 * GraphBench is a benchmark suite for
 *      microarchitectural simulation of streaming graph workloads
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
#include <stdint.h>

#include "stinger.h"
#include "xmalloc.h"
#include "hooks.h"

int
load_raw_benchmark_data_into_stinger(stinger_t * S, char * filename)
{

    FILE* in = fopen(filename, "rb");
    size_t size;
    fread(&size, sizeof(size_t), 1, in);
    S = malloc(size);
    if (!fread(S, sizeof(stinger_t), 1, in))
    {
        printf("Failed to load raw graph! Possible STINGER version mismatch.");
        return -1;
    }
    fclose(in);

    return 0;
}

int
load_benchmark_data_into_stinger (stinger_t * S, char * filename, char hooks)
{
    int64_t type = 0;
    int64_t weight = 10;
    int64_t timestamp = 1;

    FILE *fp = fopen(filename, "r");

    if (fp == NULL) {
        printf("ERROR opening file: %s\n", filename);
        exit(-1);
    }

    int64_t src, dst;
    int64_t num_edges = 0;

    printf("Loading: %s\n", filename);

    while ( EOF != fscanf(fp, "%ld %ld\n", &src, &dst) ) {
        if (src < 0 && dst == 0) {  // delete vertex
            stinger_remove_vertex (S, -src);
        } else if (src < 0) {
            stinger_remove_edge(S,type,-src,dst);
        } else { // insert edge
            stinger_insert_edge_pair (S, type, src, dst, weight, timestamp);
            num_edges++;
        }
    }

    fclose(fp);

    // consistency check
    printf("\n");
    printf("\tNumber of edges read: %ld\n", num_edges);
    printf("\tNumber of STINGER vertices: %ld\n", stinger_num_active_vertices(S));
    printf("\tNumber of STINGER edges: %ld\n", stinger_total_edges(S));
    //printf("\tConsistency: %ld (should be 0)\n", (long) stinger_consistency_check(S, STINGER_MAX_LVERTICES));

    return 0;
}


int
print_fragmentation_stats (stinger_t * S, int64_t nv)
{
  struct stinger_fragmentation_t * stats = xmalloc(sizeof(struct stinger_fragmentation_t));
  stinger_fragmentation (S, nv, stats);

  printf("\n");
  printf("\tNumber of holes: %ld\n", stats->num_empty_edges);
  printf("\tNumber of fragmented edge blocks: %ld\n", stats->num_fragmented_blocks);
  printf("\tNumber of edge blocks in use: %ld\n", stats->edge_blocks_in_use);
  printf("\tNumber of empty edge blocks: %ld\n", stats->num_empty_blocks);
  printf("\tEdge block fill percentage: %f\n", stats->fill_percent);
  printf("\tAverage number of edges per block: %f\n", stats->avg_number_of_edges);

  free(stats);
  return 0;
}
