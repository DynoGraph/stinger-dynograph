#include <stdint.h>

#include <stinger_bench.h>
#include <hooks.h>

#include <stinger.h>
#include <pagerank.h>

int main(int argc, char **argv)
{
    // Create the stinger data structure
    stinger_t * S = stinger_new();

    // Load raw data in from file
    load_benchmark_data_into_stinger (S, argv[1], 0);

    // Get number of vertices
    uint64_t num_vertices = stinger_max_active_vertex(S) + 1;

    // Allocate data structures for pagerank
    double * pagerank_scores = xmalloc (num_vertices * sizeof(int64_t));
    double * pagerank_scores_tmp = xmalloc (num_vertices * sizeof(int64_t));

    // Run the benchmark
    bench_start();
    page_rank (S, num_vertices, pagerank_scores, pagerank_scores_tmp, 1e-8, 0.85, 100);
    bench_end();

    // Clean up
    free (pagerank_scores);
    free (pagerank_scores_tmp);
    stinger_free_all (S);
    return 0;
}
