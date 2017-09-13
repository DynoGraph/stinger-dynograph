#pragma once


#include <cinttypes>
#include <vector>
#include <string>
#include <memory>
#include <sstream>
#include <iostream>
#include <assert.h>
#include <algorithm>

#include "args.h"
#include "idataset.h"
#include "alg_data_manager.h"
#include "dynamic_graph.h"
#include "logger.h"
#include <hooks.h>

namespace DynoGraph {

std::shared_ptr<IDataset>
create_dataset(const Args &args);

std::shared_ptr<Batch>
get_preprocessed_batch(int64_t batchId, IDataset &dataset, Args::SORT_MODE sort_mode);

bool
enable_algs_for_batch(int64_t batch_id, int64_t num_batches, int64_t num_epochs);

std::vector<int64_t>
load_sources_from_file(std::string path, int64_t max_vertex_id);


class Benchmark {

protected:
    Args args;
    std::shared_ptr<IDataset> dataset;
    int64_t max_vertex_id;
    AlgDataManager alg_data_manager;
    std::vector<int64_t> sources;
    Logger& logger;
    Hooks& hooks;

public:

    /* Initializes the benchmark, including the graph dataset and other
     * auxiliary data structures for recording results
     */
    Benchmark(Args& args);

    template<typename graph_t>
    void
    run_dynamic()
    {
        // Initialize the graph data structure
        graph_t graph(args, max_vertex_id);

        // Step through one batch at a time
        // Epoch will be incremented as necessary
        int64_t epoch = 0;
        int64_t num_batches = dataset->getNumBatches();
        for (int64_t batch_id = 0; batch_id < num_batches; ++batch_id)
        {
            hooks.set_attr("batch", batch_id);
            hooks.set_attr("epoch", epoch);

            // Batch preprocessing (preprocess)
            hooks.region_begin("preprocess");
            std::shared_ptr<DynoGraph::Batch> batch = get_preprocessed_batch(batch_id, *dataset, args.sort_mode);
            hooks.region_end();

            int64_t threshold = dataset->getTimestampForWindow(batch_id);
            graph.before_batch(*batch, threshold);

            // Edge deletion benchmark (deletions)
            if (args.window_size != 1.0)
            {
                logger << "Deleting edges older than " << threshold << "\n";
                hooks.set_stat("num_vertices", graph.get_num_vertices());
                hooks.set_stat("num_edges", graph.get_num_edges());
                hooks.region_begin("deletions");
                graph.delete_edges_older_than(threshold);
                hooks.region_end();
            }

            // Edge insertion benchmark (insertions)
            logger << "Inserting batch " << batch_id << "\n";
            hooks.set_stat("num_vertices", graph.get_num_vertices());
            hooks.set_stat("num_edges", graph.get_num_edges());
            hooks.region_begin("insertions");
            graph.insert_batch(*batch);
            hooks.region_end();

            // Graph algorithm benchmarks
            if (enable_algs_for_batch(batch_id, num_batches, args.num_epochs))
            {
                for (int64_t alg_trial = 0; alg_trial < args.num_alg_trials; ++alg_trial)
                {
                    // When we do multiple trials, algs should start with the same data each time
                    if (alg_trial != 0) { alg_data_manager.rollback(); }

                    // Run each alg
                    for (std::string alg_name : args.alg_names)
                    {
                        if (args.sources_path.empty()) {
                            // Pick source vertex(s)
                            int64_t num_sources;
                            if (alg_name == "bfs" || alg_name == "sssp") { num_sources = 64; }
                            else if (alg_name == "bc") { num_sources = 128; }
                            else { num_sources = 0; }
                            sources = graph.get_high_degree_vertices(num_sources);
                            if (sources.size() == 1) {
                                hooks.set_stat("source_vertex", sources[0]);
                            }
                        }

                        logger << "Running " << alg_name << " for epoch " << epoch << "\n";
                        hooks.set_stat("alg_trial", alg_trial);
                        hooks.set_stat("num_vertices", graph.get_num_vertices());
                        hooks.set_stat("num_edges", graph.get_num_edges());
                        hooks.region_begin(alg_name);
                        graph.update_alg(alg_name, sources, alg_data_manager.get_data_for_alg(alg_name));
                        hooks.region_end();
                    }
                }
                alg_data_manager.dump(epoch);
                alg_data_manager.next_epoch();
                epoch += 1;
                assert(epoch <= args.num_epochs);
            }
        }
        assert(epoch == args.num_epochs);
        // Reset dataset for next trial
        dataset->reset();
    }

    template<typename graph_t>
    void
    run_static()
    {
        DynamicGraph * graph;

        // Step through one batch at a time
        // Epoch will be incremented as necessary
        int64_t epoch = 0;
        int64_t num_batches = dataset->getNumBatches();
        for (int64_t batch_id = 0; batch_id < num_batches; ++batch_id)
        {
            hooks.set_attr("batch", batch_id);
            hooks.set_attr("epoch", epoch);

            if (enable_algs_for_batch(batch_id, num_batches, args.num_epochs))
            {
                logger << "Generating graph snapshot\n";

                // This batch will be a cumulative, filtered snapshot of all the edges in previous batches
                hooks.region_begin("preprocess");
                std::shared_ptr<DynoGraph::Batch> batch = get_preprocessed_batch(batch_id, *dataset, args.sort_mode);
                hooks.region_end();

                logger << "Initializing graph for epoch " << epoch << "\n";

                // Initialize the graph data structure
                hooks.region_begin("construct");
                graph = new graph_t(args, max_vertex_id, *batch);
                hooks.region_end();

                // Graph algorithm benchmarks
                for (int64_t alg_trial = 0; alg_trial < args.num_alg_trials; ++alg_trial)
                {
                    // When we do multiple trials, algs should start with the same data each time
                    if (alg_trial != 0) { alg_data_manager.rollback(); }

                    // Run each alg
                    for (std::string alg_name : args.alg_names)
                    {
                        if (args.sources_path.empty()) {
                            // Pick source vertex(s)
                            int64_t num_sources;
                            if (alg_name == "bfs" || alg_name == "sssp") { num_sources = 64; }
                            else if (alg_name == "bc") { num_sources = 128; }
                            else { num_sources = 0; }
                            sources = graph->get_high_degree_vertices(num_sources);
                            if (sources.size() == 1) {
                                hooks.set_stat("source_vertex", sources[0]);
                            }
                        }

                        logger << "Running " << alg_name << " for epoch " << epoch << "\n";
                        hooks.set_stat("alg_trial", alg_trial);
                        hooks.set_stat("num_vertices", graph->get_num_vertices());
                        hooks.set_stat("num_edges", graph->get_num_edges());
                        hooks.region_begin(alg_name);
                        graph->update_alg(alg_name, sources, alg_data_manager.get_data_for_alg(alg_name));
                        hooks.region_end();
                    }
                }
                alg_data_manager.dump(epoch);
                alg_data_manager.next_epoch();
                epoch += 1;
                assert(epoch <= args.num_epochs);

                // Destroy the graph so we can reinitialize before the next epoch
                hooks.region_begin("destroy");
                delete graph;
                hooks.region_end();
            }
        }
        assert(epoch == args.num_epochs);
        // Reset dataset for next trial
        dataset->reset();
    }

    template<typename graph_t>
    static void
    run(int argc, char **argv)
    {
        Args args = Args::parse(argc, argv);
        Benchmark benchmark(args);
        for (int64_t trial = 0; trial < args.num_trials; trial++)
        {
            Hooks::getInstance().set_attr("trial", trial);
            if (args.sort_mode == Args::SORT_MODE::SNAPSHOT) {
                benchmark.run_static<graph_t>();
            } else if (args.sort_mode == Args::SORT_MODE::UNSORTED) {
                benchmark.run_dynamic<graph_t>();
            }
        }
    }

}; // end class Benchmark

} // end namespace DynoGraph
