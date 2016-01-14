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
#include <dynograph_util.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

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

void
dynograph_message(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[DynoGraph] ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

void
dynograph_error(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[DynoGraph] ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}

struct dynograph_preloaded_edge_batch*
dynograph_preload_batch(const char* path)
{
    // Allocate memory for each line in the file
    int64_t num_lines = count_lines(path);
    struct dynograph_preloaded_edge_batch *batch = malloc(
        sizeof(struct dynograph_preloaded_edge_batch) +
        sizeof(struct dynograph_preloaded_edge) * num_lines
    );
    batch->num_edges = num_lines;
    batch->directed = false; // TODO detect directed/undirected somewhere
    // Load the batch
    dynograph_message("Preloading a batch of %ld edges from %s", num_lines, path);
    FILE* fp = fopen(path, "r");
    int rc = 0;
    for (struct dynograph_preloaded_edge* e = &batch->edges[0]; rc != EOF; ++e)
    {
        rc = fscanf(fp, "%ld %ld %ld %ld\n", &e->src, &e->dst, &e->weight, &e->timestamp);
    }
    fclose(fp);
    return batch;
}

struct dynograph_preloaded_edge_batches*
dynograph_preload_batches(const char* base_path)
{
    // Use glob to get a list of all the batches
    const char* suffix = ".batch???.graph.el";
    char* pattern = malloc((strlen(base_path) + strlen(suffix) + 1) * sizeof(char));
    strcpy(pattern, base_path);
    strcat(pattern, suffix);
    glob_t globbuf;
    glob(pattern, GLOB_TILDE_CHECK, NULL, &globbuf);

    // Load the batches
    struct dynograph_preloaded_edge_batches *batches = malloc(
        sizeof(struct dynograph_preloaded_edge_batches) +
        sizeof(struct dynograph_preloaded_edge_batch*) * globbuf.gl_pathc
    );
    batches->num_batches = globbuf.gl_pathc;
    for (unsigned i = 0; i < batches->num_batches; ++i)
    {
        batches->batches[i] = dynograph_preload_batch(globbuf.gl_pathv[i]);
    }
    // Clean up
    globfree(&globbuf);
    free(pattern);
    return batches;
}

void
dynograph_free_batches(struct dynograph_preloaded_edge_batches *batches)
{
    for (int64_t i = 0; i < batches->num_batches; ++i)
    {
        free(batches->batches[i]);
    }
    free(batches);
}

int64_t
dynograph_get_timestamp_for_window(struct dynograph_preloaded_edge_batches *batches, int64_t batch_id, int64_t window_size)
{
    int64_t modified_after = INT64_MIN;
    if (batch_id >= window_size)
    {
        modified_after = batches->batches[batch_id - window_size]->edges[0].timestamp;
    }
    return modified_after;
}

void
dynograph_insert_preloaded_batch(stinger_t * S, struct dynograph_preloaded_edge_batch* batch)
{
    dynograph_message("Inserting batch of %ld edges", batch->num_edges);
    int type = 0;
    OMP("omp parallel for")
    for (unsigned i = 0; i < batch->num_edges; ++i)
    {
        struct dynograph_preloaded_edge* e = &batch->edges[i];
        if (batch->directed)
        {
            stinger_insert_edge     (S, type, e->src, e->dst, e->weight, e->timestamp);
        } else { // undirected
            stinger_insert_edge_pair(S, type, e->src, e->dst, e->weight, e->timestamp);
        }
    }
}

void
dynograph_load_graph (stinger_t * S, const char * name)
{
    const char* suffix = ".graph.bin";
    char* filename = malloc( strlen(name) + strlen(suffix) + 1);
    strcpy(filename, name);
    strcat(filename, suffix);
    dynograph_message("Loading: %s", filename);

    uint64_t maxVtx;
    int rc = stinger_open_from_file(filename, S, &maxVtx);

    if (rc == -1)
    {
        dynograph_error("Failed to open %s, exiting.", filename);
    }
    free(filename);
}

// Counts the number of edges that satsify the filter
int64_t
dynograph_filtered_edge_count (struct stinger * S, int64_t nv, int64_t modified_after)
{
    int64_t num_edges = 0;

    OMP ("omp parallel for reduction(+:num_edges)")
    for (int64_t v = 0; v < nv; v++) {
        STINGER_FORALL_OUT_EDGES_OF_VTX_MODIFIED_AFTER_BEGIN (S, v, modified_after) {
            num_edges += 1;
        } STINGER_FORALL_OUT_EDGES_OF_VTX_MODIFIED_AFTER_END();
    }

    return num_edges;
}

void dynograph_print_graph_size(stinger_t *S, int64_t nv, int64_t modified_after)
{
    printf("{\"num_vertices\":%ld,\"num_edges\":%ld,\"num_filtered_edges\":%ld}\n",
        nv,
        stinger_total_edges(S),
        dynograph_filtered_edge_count(S, nv, modified_after)
    );
}

int
dynograph_print_fragmentation_stats (stinger_t * S, int64_t nv)
{
  struct stinger_fragmentation_t * stats = xmalloc(sizeof(struct stinger_fragmentation_t));
  stinger_fragmentation (S, nv, stats);

  dynograph_message("\tNumber of holes: %ld", stats->num_empty_edges);
  dynograph_message("\tNumber of fragmented edge blocks: %ld", stats->num_fragmented_blocks);
  dynograph_message("\tNumber of edge blocks in use: %ld", stats->edge_blocks_in_use);
  dynograph_message("\tNumber of empty edge blocks: %ld", stats->num_empty_blocks);
  dynograph_message("\tEdge block fill percentage: %f", stats->fill_percent);
  dynograph_message("\tAverage number of edges per block: %f", stats->avg_number_of_edges);

  free(stats);
  return 0;
}
