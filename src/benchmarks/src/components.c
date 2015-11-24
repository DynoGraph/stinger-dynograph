#include <stdint.h>

#include <stinger_bench.h>
#include <hooks.h>

#include <stinger.h>
#include <static_components.h>

int main(int argc, char **argv)
{
    // Create the stinger data structure
    stinger_t * S = stinger_new();

    // Load raw data in from file
    load_graph(S, argv[1]);
    struct preloaded_edge_batches* batches = preload_batches(argv[1]);

    // Pre-allocate output data structures
    uint64_t num_vertices = stinger_max_nv(S);
    int64_t *component_map = xmalloc (num_vertices * sizeof(int64_t));

    // Run the benchmark
    bench_start();
    parallel_shiloach_vishkin_components(S, num_vertices, component_map);
    for (int64_t i = 0; i < batches->num_batches; ++i)
    {
        bench_region();
        insert_preloaded_batch(S, batches->batches[i]);
        bench_region();
        parallel_shiloach_vishkin_components(S, num_vertices, component_map);
    }

    bench_end();

    // Clean up
    free (component_map);
    stinger_free_all (S);
    return 0;
}
