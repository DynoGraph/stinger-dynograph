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
    load_benchmark_data_into_stinger (S, argv[1], 0);

    // Get number of vertices
    uint64_t num_vertices = stinger_max_active_vertex(S) + 1;

    // Pre-allocate output data structures
    double * bc = xmalloc(num_vertices * sizeof(int64_t));
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
    bench_end();

    // Clean up
    free (bc);
    free (found_count);
    stinger_free_all (S);
    return 0;
}
