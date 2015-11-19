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
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stinger.h>
#include "xmalloc.h"

stinger_t *
bench_load_graph_with_timestamps(stinger_t * S, const char * filename, bool directed)
{
    FILE *in = fopen(filename, "r");
    if (in == NULL)
    {
        printf("ERROR opening file: %s\n", filename);
        exit(-1);
    }
    uint64_t max_nv = stinger_max_nv(S);
    int64_t src, dst, weight, ts;
    while ( EOF != fscanf(in, "%ld %ld %ld %ld\n", &src, &dst, &weight, &ts) )
    {
        if (src >= max_nv || dst >= max_nv)
        {
            printf("Too many vertices: max_nv = %lu, src = %lu, dst = %lu\n", max_nv, src, dst);
        }
        if (directed) {
            stinger_insert_edge     (S, 0, src, dst, weight, ts);
        } else {
            stinger_insert_edge_pair(S, 0, src, dst, weight, ts);
        }
    }
    fclose(in);
    return S;
}


int
main (int argc, char ** argv)
{
    if (argc < 3)
    {
        printf("Usage: %s input.graph.el output.graph.bin\n", argv[0]);
        exit(1);
    }

    // Convert undirected graph to binary
    stinger_t * S = stinger_new();
    bool directed = false;
    printf("Loading: %s\n", argv[1]);
    bench_load_graph_with_timestamps(S, argv[1], directed);
    printf("Saving: %s\n", argv[2]);
    stinger_save_to_file(S, stinger_max_active_vertex(S), argv[2]);
    stinger_free_all (S);
    return 0;
}

