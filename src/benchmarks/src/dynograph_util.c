#include <dynograph_util.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

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

struct dynograph_dataset*
dynograph_load_edges_binary(const char* path, int64_t num_batches)
{
    dynograph_message("Checking file size of %s...", path);
    FILE* fp = fopen(path, "rb");
    struct stat st;
    if (stat(path, &st) != 0)
    {
        dynograph_error("Failed to stat %s", path);
    }
    int64_t num_edges = st.st_size / sizeof(struct dynograph_edge);

    struct dynograph_dataset *dataset = malloc(
        sizeof(struct dynograph_dataset) +
        sizeof(struct dynograph_edge) * num_edges
    );
    if (dataset == NULL)
    {
        dynograph_error("Failed to allocate memory for %ld edges", num_edges);
    }
    dataset->num_edges = num_edges;
    dataset->num_batches = num_batches;
    dataset->directed = true; // FIXME detect this somehow

    dynograph_message("Preloading %ld %s edges from %s...", num_edges, dataset->directed ? "directed" : "undirected", path);

    size_t rc = fread(&dataset->edges[0], sizeof(struct dynograph_edge), num_edges, fp);
    if (rc != num_edges)
    {
        dynograph_error("Failed to load graph from %s",path);
    }
    fclose(fp);
    return dataset;
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

struct dynograph_dataset*
dynograph_load_edges_ascii(const char* path, int64_t num_batches)
{
    dynograph_message("Counting lines in %s...", path);
    int64_t num_edges = dynograph_count_lines(path);

    struct dynograph_dataset *dataset = malloc(
        sizeof(struct dynograph_dataset) +
        sizeof(struct dynograph_edge) * num_edges
    );
    if (dataset == NULL)
    {
        dynograph_error("Failed to allocate memory for %ld edges", num_edges);
    }
    dataset->num_edges = num_edges;
    dataset->num_batches = num_batches;
    dataset->directed = true; // FIXME detect this somehow

    dynograph_message("Preloading %ld %s edges from %s...", num_edges, dataset->directed ? "directed" : "undirected", path);

    FILE* fp = fopen(path, "r");
    int rc = 0;
    for (struct dynograph_edge* e = &dataset->edges[0]; rc != EOF; ++e)
    {
        rc = fscanf(fp, "%ld %ld %ld %ld\n", &e->src, &e->dst, &e->weight, &e->timestamp);
    }
    fclose(fp);
    return dataset;
}

bool
dynograph_file_is_binary(const char* path)
{
    const char* suffix = ".graph.bin";
    size_t path_len = strlen(path);
    size_t suffix_len = strlen(suffix);
    return !strcmp(path + path_len - suffix_len, suffix);
}

struct dynograph_dataset*
dynograph_load_dataset(const char* path, int64_t num_batches)
{
    // Sanity check
    if (num_batches < 1)
    {
        dynograph_error("Need at least one batch");
    }

    if (dynograph_file_is_binary(path))
    {
        return dynograph_load_edges_binary(path, num_batches);
    } else {
        return dynograph_load_edges_ascii(path, num_batches);
    }
}

int64_t
dynograph_get_timestamp_for_window(const struct dynograph_dataset *dataset, int64_t batch_id, int64_t window_size)
{
    int64_t modified_after = INT64_MIN;
    if (batch_id > window_size)
    {
        // Intentionally rounding down here
        // TODO variable number of edges per batch
        int64_t edges_per_batch = dataset->num_edges / dataset->num_batches;
        int64_t startEdge = (batch_id - window_size) * edges_per_batch;
        modified_after = dataset->edges[startEdge].timestamp;
    }
    return modified_after;
}

struct dynograph_edge_batch
dynograph_get_batch(const struct dynograph_dataset* dataset, int64_t batch_id)
{
    if (batch_id >= dataset->num_batches)
    {
        dynograph_error("Batch %i does not exist!", batch_id);
    }
    // Intentionally rounding down here
    // TODO variable number of edges per batch
    int64_t edges_per_batch = dataset->num_edges / dataset->num_batches;
    size_t offset = batch_id * edges_per_batch;

    struct dynograph_edge_batch batch = {
        .num_edges = edges_per_batch,
        .directed = dataset->directed,
        .edges = dataset->edges + offset
    };
    return batch;
}

void
dynograph_free_dataset(struct dynograph_dataset * dataset)
{
    free(dataset);
}
