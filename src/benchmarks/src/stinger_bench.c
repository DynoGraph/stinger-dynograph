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
#include <stinger_bench.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>

int64_t
count_lines(const char* path)
{
    FILE* fp = fopen(path, "r");
    int64_t lines = 0;
    while(!feof(fp))
    {
        int ch = fgetc(fp);
        if(ch == '\n')
        {
            lines++;
        }
    }
    fclose(fp);
    return lines;
}

struct preloaded_edge_batch*
preload_batch(const char* path)
{
    // Allocate memory for each line in the file
    int64_t num_lines = count_lines(path);
    struct preloaded_edge_batch *batch = malloc(
        sizeof(struct preloaded_edge_batch) +
        sizeof(struct preloaded_edge) * num_lines
    );
    batch->num_edges = num_lines;
    batch->directed = false; // TODO detect directed/undirected somewhere
    // Load the batch
    printf("[DynoGraph] Preloading a batch of %ld edges from %s\n", num_lines, path);
    FILE* fp = fopen(path, "r");
    int rc = 0;
    for (struct preloaded_edge* e = &batch->edges[0]; rc != EOF; ++e)
    {
        rc = fscanf(fp, "%ld %ld %ld %ld\n", &e->src, &e->dst, &e->weight, &e->timestamp);
    }
    fclose(fp);
    return batch;
}

struct preloaded_edge_batches*
preload_batches(const char* base_path)
{
    // Use glob to get a list of all the batches
    const char* suffix = ".batch???.graph.el";
    char* pattern = malloc((strlen(base_path) + strlen(suffix) + 1) * sizeof(char));
    strcpy(pattern, base_path);
    strcat(pattern, suffix);
    glob_t globbuf;
    glob(pattern, GLOB_TILDE_CHECK, NULL, &globbuf);

    // Load the batches
    struct preloaded_edge_batches *batches = malloc(
        sizeof(struct preloaded_edge_batches) +
        sizeof(struct preloaded_edge_batch*) * globbuf.gl_pathc
    );
    batches->num_batches = globbuf.gl_pathc;
    for (unsigned i = 0; i < batches->num_batches; ++i)
    {
        batches->batches[i] = preload_batch(globbuf.gl_pathv[i]);
    }
    // Clean up
    globfree(&globbuf);
    free(pattern);
    return batches;
}

void
insert_preloaded_batch(stinger_t * S, struct preloaded_edge_batch* batch)
{
    printf("[DynoGraph] Inserting batch of %ld edges\n", batch->num_edges);
    int type = 0;
    for (unsigned i = 0; i < batch->num_edges; ++i)
    {
        struct preloaded_edge* e = &batch->edges[i];
        if (batch->directed)
        {
            stinger_insert_edge     (S, type, e->src, e->dst, e->weight, e->timestamp);
        } else { // undirected
            stinger_insert_edge_pair(S, type, e->src, e->dst, e->weight, e->timestamp);
        }
    }
    free(batch);
}

void
load_graph (stinger_t * S, const char * name)
{
    const char* suffix = ".graph.bin";
    char* filename = malloc( strlen(name) + strlen(suffix) + 1);
    strcpy(filename, name);
    strcat(filename, suffix);
    printf("[DynoGraph] Loading: %s\n", filename);

    uint64_t maxVtx;
    int rc = stinger_open_from_file(filename, S, &maxVtx);

    if (rc == -1)
    {
        printf("[DynoGraph] Failed to open %s, exiting.", filename);
        exit(1);
    }
    free(filename);
}

// Deprecated
int
load_edgelist (stinger_t * S, const char * filename)
{
    int64_t type = 0;
    int64_t weight = 10;
    int64_t timestamp = 1;

    FILE *fp = fopen(filename, "r");

    if (fp == NULL) {
        printf("[DynoGraph] ERROR opening file: %s\n", filename);
        exit(-1);
    }

    int64_t src, dst;
    int64_t num_edges = 0;

    while ( EOF != fscanf(fp, "%ld %ld %ld %ld\n", &src, &dst, &weight, &timestamp) ) {
        if (src < 0 && dst == 0) {  // delete vertex
            stinger_remove_vertex (S, -src);
        } else if (src < 0) {
            stinger_remove_edge(S,type,-src,dst);
        } else { // insert edge
            // Change to incr_edge, are these directed graphs?
            stinger_insert_edge_pair (S, type, src, dst, weight, timestamp);
            num_edges++;
        }
    }

    fclose(fp);

    // consistency check
    printf("\n");
    printf("\t[DynoGraph] Number of edges read: %ld\n", num_edges);
    //printf("\tNumber of STINGER vertices: %ld\n", stinger_num_active_vertices(S));
    //printf("\tNumber of STINGER edges: %ld\n", stinger_total_edges(S));
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
