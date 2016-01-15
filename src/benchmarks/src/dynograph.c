#include <stdint.h>

#include <dynograph_util.h>
#include <hooks.h>

#include <stinger.h>
#include <bfs.h>
#include <betweenness.h>
#include <clustering.h>
#include <static_components.h>
#include <kcore.h>
#include <pagerank.h>

struct dynograph_args
{
    const char* alg_name;
    const char* input_path;
    int64_t window_size;
    int64_t num_batches;
    int64_t num_trials;
};

struct dynograph_args
dynograph_get_args(int argc, char **argv)
{
    struct dynograph_args args;
    if (argc != 5)
    {
        dynograph_error("Usage: alg_name input_path num_batches window_size");
    }

    args.alg_name = argv[1];
    args.input_path = argv[2];
    args.num_batches = atoll(argv[3]);
    args.window_size = atoll(argv[4]);
    if (args.num_batches < 1 || args.window_size < 1)
    {
        dynograph_error("num_batches and window_size must be positive");
    }
    args.num_trials = 1;
    return args;
}

struct dynograph_benchmark
{
    const char *name;
    int64_t data_per_vertex;
    // Number of optional args?
} dynograph_benchmarks[] = {
    {"all", 4}, // Must be = max(data_per_vertex) of all other algorithms
    {"bfs", 4},
    {"bfs-do", 4},
    {"betweenness", 2},
    {"clustering", 1},
    {"components", 1},
    {"kcore", 2},
    {"pagerank", 2}
};

struct dynograph_benchmark *
dynograph_get_benchmark(const char *name)
{
    struct dynograph_benchmark *b = NULL;
    for (int i = 0; i < sizeof(dynograph_benchmarks) / sizeof(dynograph_benchmarks[0]); ++i)
    {
        if (!strcmp(dynograph_benchmarks[i].name, name))
        {
            b = &dynograph_benchmarks[i];
            break;
        }
    }
    if (b == NULL)
    {
        dynograph_error("Benchmark '%s' does not exist!", name);
    }
    return b;
}

void dynograph_run_benchmark(const char *alg_name, stinger_t * S, int64_t num_vertices, void *alg_data, int64_t modified_after)
{
    dynograph_message("Running %s...", alg_name);
    int64_t max_nv = stinger_max_nv(S);

    if (!strcmp(alg_name, "all"))
    {
        for (int i = 1; i < sizeof(dynograph_benchmarks) / sizeof(dynograph_benchmarks[0]); ++i)
        {
            dynograph_run_benchmark(dynograph_benchmarks[i].name, S, num_vertices, alg_data, modified_after);
        }
    }
    else if (!strcmp(alg_name, "bfs"))
    {
        int64_t * marks = (int64_t*)alg_data + 0 * max_nv;
        int64_t * queue = (int64_t*)alg_data + 1 * max_nv;
        int64_t * Qhead = (int64_t*)alg_data + 2 * max_nv;
        int64_t * level = (int64_t*)alg_data + 3 * max_nv;
        int64_t levels;
        int64_t source_vertex = 3; // FIXME Get this from the command line
        hooks_region_begin();
        levels = parallel_breadth_first_search (S, num_vertices, source_vertex, marks, queue, Qhead, level, modified_after);
        hooks_region_end();
        if (levels < 5)
        {
            dynograph_message("WARNING: Breadth-first search was only %ld levels. Consider choosing a different source vertex.", levels);
        }
    }
    else if (!strcmp(alg_name, "bfs-do"))
    {
        int64_t * marks = (int64_t*)alg_data + 0 * max_nv;
        int64_t * queue = (int64_t*)alg_data + 1 * max_nv;
        int64_t * Qhead = (int64_t*)alg_data + 2 * max_nv;
        int64_t * level = (int64_t*)alg_data + 3 * max_nv;
        int64_t levels;
        int64_t source_vertex = 3; // FIXME Get this from the command line
        hooks_region_begin();
        levels = direction_optimizing_parallel_breadth_first_search (S, num_vertices, source_vertex, marks, queue, Qhead, level);
        hooks_region_end();
        if (levels < 5)
        {
            dynograph_message("WARNING: Breadth-first search was only %ld levels. Consider choosing a different source vertex.", levels);
        }
    }
    else if (!strcmp(alg_name, "betweenness"))
    {
        double *bc =            (double*) alg_data + 0 * max_nv;
        int64_t *found_count =  (int64_t*)alg_data + 1 * max_nv;
        int64_t num_samples = 256; // FIXME Allow override from command line
        hooks_region_begin();
        sample_search(S, num_vertices, num_samples, bc, found_count);
        hooks_region_end();
    }
    else if (!strcmp(alg_name, "clustering"))
    {
        int64_t *num_triangles = (int64_t*) alg_data + 0 * max_nv;
        hooks_region_begin();
        count_all_triangles(S, num_triangles);
        hooks_region_end();
    }
    else if (!strcmp(alg_name, "components"))
    {
        int64_t *component_map = (int64_t*) alg_data + 0 * max_nv;
        hooks_region_begin();
        parallel_shiloach_vishkin_components(S, num_vertices, component_map);
        hooks_region_end();
    }
    else if (!strcmp(alg_name, "kcore"))
    {
        int64_t *labels = (int64_t*) alg_data + 0 * max_nv;
        int64_t *counts = (int64_t*) alg_data + 1 * max_nv;
        int64_t k = 0;
        hooks_region_begin();
        kcore_find(S, labels, counts, num_vertices, &k);
        hooks_region_end();
    }
    else if (!strcmp(alg_name, "pagerank"))
    {
        double * pagerank_scores =      (double*)alg_data + 0 * max_nv;
        double * pagerank_scores_tmp =  (double*)alg_data + 1 * max_nv;
        hooks_region_begin();
        page_rank_directed(S, num_vertices, pagerank_scores, pagerank_scores_tmp, 1e-8, 0.85, 100, modified_after);
        hooks_region_end();
    }
    else
    {
        dynograph_error("Algorithm %s not implemented!", alg_name);
    }
}

int main(int argc, char **argv)
{
    // Process command line arguments
    struct dynograph_args args = dynograph_get_args(argc, argv);
    // Create the stinger data structure
    stinger_t * S = stinger_new();
    // Load graph data in from the file in batches
    struct dynograph_preloaded_edges* edges = dynograph_preload_edges(args.input_path, args.num_batches);
    // Look up the algorithm that will be benchmarked
    struct dynograph_benchmark *b = dynograph_get_benchmark(args.alg_name);
    // Allocate data structures for the algorithm
    uint64_t num_vertices = stinger_max_nv(S);
    void *alg_data = xcalloc(sizeof(int64_t) * b->data_per_vertex, num_vertices);

    int64_t numTrials = args.num_trials;
    for (int64_t trial = 0; trial < numTrials; trial++)
    {
        for (int64_t i = 0; i < edges->num_batches; ++i)
        {
            dynograph_insert_preloaded_batch(S, edges, i);
            int64_t modified_after = dynograph_get_timestamp_for_window(edges, i, args.window_size);
            num_vertices = stinger_max_active_vertex(S) + 1; // TODO faster way to get this?
            dynograph_run_benchmark(b->name, S, num_vertices, alg_data, modified_after);
            dynograph_print_graph_size(S, num_vertices, modified_after);
        }
        // TODO clear out stinger data structure
    }

    // Clean up
    free(alg_data);
    free(edges);
    stinger_free_all (S);
    return 0;
}
