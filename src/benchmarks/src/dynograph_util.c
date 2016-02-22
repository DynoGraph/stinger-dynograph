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

struct dynograph_preloaded_edges*
dynograph_preload_edges_binary(const char* path, int64_t num_batches)
{
    dynograph_message("Checking file size of %s...", path);
    FILE* fp = fopen(path, "rb");
    struct stat st;
    if (stat(path, &st) != 0)
    {
        dynograph_error("Failed to stat %s", path);
    }
    int64_t num_edges = st.st_size / sizeof(struct dynograph_preloaded_edge);

    struct dynograph_preloaded_edges *edges = malloc(
        sizeof(struct dynograph_preloaded_edges) +
        sizeof(struct dynograph_preloaded_edge) * num_edges
    );
    edges->num_edges = num_edges;
    edges->num_batches = num_batches;
    edges->edges_per_batch = num_edges / num_batches; // Intentionally rounding down here
    edges->directed = true; // FIXME detect this somehow

    dynograph_message("Preloading %ld %s edges from %s...", edges->num_edges, edges->directed ? "directed" : "undirected", path);

    size_t rc = fread(&edges->edges, sizeof(struct dynograph_preloaded_edge), num_edges, fp);
    if (rc != num_edges)
    {
        dynograph_error("Failed to load graph from %s",path);
    }
    fclose(fp);
    return edges;
}

int64_t
dynograph_count_lines(const char* path)
{
    FILE* fp = fopen(path, "r");
    if (fp == NULL)
    {
        dynograph_error("Failed to open %s", path);
    }
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

struct dynograph_preloaded_edges*
dynograph_preload_edges_ascii(const char* path, int64_t num_batches)
{
    dynograph_message("Counting lines in %s...", path);
    int64_t num_edges = dynograph_count_lines(path);

    struct dynograph_preloaded_edges *edges = malloc(
        sizeof(struct dynograph_preloaded_edges) +
        sizeof(struct dynograph_preloaded_edge) * num_edges
    );
    edges->num_edges = num_edges;
    edges->num_batches = num_batches;
    edges->edges_per_batch = num_edges / num_batches; // Intentionally rounding down here
    edges->directed = true; // FIXME detect this somehow

    dynograph_message("Preloading %ld %s edges from %s...", edges->num_edges, edges->directed ? "directed" : "undirected", path);

    FILE* fp = fopen(path, "r");
    int rc = 0;
    // TODO buffer manually to avoid doing so many calls to fscanf
    for (struct dynograph_preloaded_edge* e = &edges->edges[0]; rc != EOF; ++e)
    {
        rc = fscanf(fp, "%ld %ld %ld %ld\n", &e->src, &e->dst, &e->weight, &e->timestamp);
    }
    fclose(fp);
    return edges;
}

bool
dynograph_file_is_binary(const char* path)
{
    const char* suffix = ".graph.bin";
    size_t path_len = strlen(path);
    size_t suffix_len = strlen(suffix);
    return !strcmp(path + path_len - suffix_len, suffix);
}

struct dynograph_preloaded_edges*
dynograph_preload_edges(const char* path, int64_t num_batches)
{
    // Sanity check
    if (num_batches < 1)
    {
        dynograph_error("Need at least one batch");
    }

    if (dynograph_file_is_binary(path))
    {
        return dynograph_preload_edges_binary(path, num_batches);
    } else {
        return dynograph_preload_edges_ascii(path, num_batches);
    }
}

int64_t
dynograph_get_timestamp_for_window(struct dynograph_preloaded_edges *edges, int64_t batch_id, int64_t window_size)
{
    int64_t modified_after = INT64_MIN;
    if (batch_id >= window_size)
    {
        int64_t startEdge = (batch_id - window_size) * edges->edges_per_batch;
        modified_after = edges->edges[startEdge].timestamp;
    }
    return modified_after;
}

void
dynograph_insert_preloaded_batch(stinger_t * S, struct dynograph_preloaded_edges* edges, int64_t batch_id)
{
    if (batch_id >= edges->num_batches)
    {
        dynograph_error("Batch %i does not exist!", batch_id);
    }
    dynograph_message("Inserting batch %ld (%ld edges)", batch_id, edges->edges_per_batch);

    int64_t begin = batch_id * edges->edges_per_batch;
    int64_t end = (batch_id+1) * edges->edges_per_batch;
    int type = 0;
    int directed = edges->directed;
    hooks_region_begin();
    OMP("omp parallel for")
    for (int64_t i = begin; i < end; ++i)
    {
        struct dynograph_preloaded_edge* e = &edges->edges[i];
        if (directed)
        {
            stinger_insert_edge     (S, type, e->src, e->dst, e->weight, e->timestamp);
        } else { // undirected
            stinger_insert_edge_pair(S, type, e->src, e->dst, e->weight, e->timestamp);
        }
    }
    hooks_region_end();
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
  // TODO dump this as JSON and integrate with print_graph_size
  dynograph_message("\tNumber of holes: %ld", stats->num_empty_edges);
  dynograph_message("\tNumber of fragmented edge blocks: %ld", stats->num_fragmented_blocks);
  dynograph_message("\tNumber of edge blocks in use: %ld", stats->edge_blocks_in_use);
  dynograph_message("\tNumber of empty edge blocks: %ld", stats->num_empty_blocks);
  dynograph_message("\tEdge block fill percentage: %f", stats->fill_percent);
  dynograph_message("\tAverage number of edges per block: %f", stats->avg_number_of_edges);

  free(stats);
  return 0;
}
