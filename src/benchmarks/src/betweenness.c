#include <stdint.h>

#include <stinger_bench.h>
#include <hooks.h>

#include <stinger.h>
#include <betweenness.h>

int main(int argc, char **argv)
{
    // Create the stinger data structure
    stinger_t * S = stinger_new();

    // Load raw data in from file
    load_graph(S, argv[1]);
    struct preloaded_edge_batches* batches = preload_batches(argv[1]);

    // Pre-allocate output data structures
    uint64_t num_vertices = stinger_max_nv(S);
    double *bc = xmalloc(num_vertices * sizeof(int64_t));
    int64_t *found_count = xmalloc(num_vertices * sizeof(int64_t));

    // Allow override of num_samples
    int64_t num_samples = 256;
    if (argc > 2)
    {
        num_samples = atol(argv[2]);
    }

    // Run the benchmark
    bench_start();
    sample_search(S, num_vertices, num_samples, bc, found_count);
    for (int64_t i = 0; i < batches->num_batches; ++i)
    {
        bench_region();
        insert_preloaded_batch(S, batches->batches[i]);
        bench_region();
        sample_search(S, num_vertices, num_samples, bc, found_count);
    }
    bench_end();

    // Clean up
    free (bc);
    free (found_count);
    stinger_free_all (S);
    return 0;
}
