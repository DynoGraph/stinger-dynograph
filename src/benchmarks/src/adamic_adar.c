#include <stdint.h>

#include <stinger_bench.h>
#include <hooks.h>

#include <stinger.h>
#include <adamic_adar.h>

int main(int argc, char **argv)
{
    // Create the stinger data structure
    stinger_t * S = stinger_new();

    // Load raw data in from file
    load_graph(S, argv[1]);

    // Allocate pointers for return values
    int64_t *candidates = NULL;
    double *scores = NULL;

    // Allow override of source
    int64_t source = 0;
    if (argc > 2)
    {
        source = atol(argv[2]);
    }

    // Run the benchmark
    bench_start();
    int64_t two_hop_size = adamic_adar(S, source, -1, &candidates, &scores);
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
