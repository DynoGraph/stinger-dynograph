#include <stdint.h>

#include <stinger_bench.h>
#include <hooks.h>

#include <stinger.h>
#include <pagerank.h>

int main(int argc, char **argv)
{
    assert(argc == 2);

    // Create the stinger data structure
    stinger_t * S = stinger_new();

    // Load graph data in from the file
    load_graph(S, argv[1]);
    struct preloaded_edge_batches* batches = preload_batches(argv[1]);

    // Pre-allocate output data structures
    uint64_t num_vertices = stinger_max_nv(S);
    double * pagerank_scores = xmalloc (num_vertices * sizeof(int64_t));
    double * pagerank_scores_tmp = xmalloc (num_vertices * sizeof(int64_t));

    // Run the benchmark
    bench_start();
    page_rank_directed(S, num_vertices, pagerank_scores, pagerank_scores_tmp, 1e-8, 0.85, 100);
    for (int64_t i = 0; i < batches->num_batches; ++i)
    {
        bench_region();
        insert_preloaded_batch(S, batches->batches[i]);
        bench_region();
        page_rank_directed(S, num_vertices, pagerank_scores, pagerank_scores_tmp, 1e-8, 0.85, 100);
    }
    bench_end();

    // Clean up
    free (pagerank_scores);
    free (pagerank_scores_tmp);
    stinger_free_all (S);
    return 0;
}
