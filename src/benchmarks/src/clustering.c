#include <stdint.h>

#include <stinger_bench.h>
#include <hooks.h>

#include <stinger.h>
#include <clustering.h>

int main(int argc, char **argv)
{
    // Create the stinger data structure
    stinger_t * S = stinger_new();

    // Load raw data in from file
    load_benchmark_data_into_stinger (S, argv[1], 0);

    // Get number of vertices
    uint64_t num_vertices = stinger_max_active_vertex(S) + 1;

    // Pre-allocate output data structures
    int64_t *num_triangles = xmalloc (num_vertices * sizeof(int64_t));

    // Run the benchmark
    bench_start();
    count_all_triangles(S, num_triangles);
    bench_end();

    // Clean up
    free (num_triangles);
    stinger_free_all (S);
    return 0;
}
