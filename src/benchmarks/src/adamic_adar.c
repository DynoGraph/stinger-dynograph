#include <stdint.h>

#include <stinger_bench.h>
#include <hooks.h>

#include <stinger.h>
#include <adamic_adar.h>

int main(int argc, char **argv)
{
    // Create the stinger data structure
    stinger_t * S = stinger_new();

    // Load graph data in from the file
    load_graph(S, argv[1]);
    struct preloaded_edge_batches* batches = preload_batches(argv[1]);

    // Allocate pointers for return values
    int64_t *candidates = NULL;
    double *scores = NULL;
    int64_t two_hop_size;

    // Allow override of source
    int64_t source = 0;
    if (argc > 2)
    {
        source = atol(argv[2]);
    }

    // Run the benchmark
    bench_start();
    two_hop_size = adamic_adar(S, source, -1, &candidates, &scores);
    for (int64_t i = 0; i < batches->num_batches; ++i)
    {
        bench_region();
        insert_preloaded_batch(S, batches->batches[i]);
        bench_region();
        two_hop_size = adamic_adar(S, source, -1, &candidates, &scores);
    }
    bench_end();

    // Check for trivial problem size
    if (two_hop_size < 100)
    {
        printf("WARNING: The two-hop neighborhood of vertex %li contains only %li vertices. Consider choosing a different source vertex.\n",
                source, two_hop_size);
    }

    // Clean up
    free (candidates);
    free (scores);
    stinger_free_all (S);
    return 0;
}
