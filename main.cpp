/*
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <vector>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <stdint.h>

#include <hooks.h>
#include <dynograph_util.hh>
#include "stinger_server.h"

using std::cerr;
using DynoGraph::msg;

int main(int argc, char **argv)
{
    // Process command line arguments
    DynoGraph::Args args(argc, argv);
    // Load graph data in from the file in batches
    DynoGraph::Dataset dataset(args);
    Hooks& hooks = Hooks::getInstance();

    for (int64_t trial = 0; trial < args.num_trials; trial++)
    {
        hooks.set_attr("trial", trial);
        // Create the stinger data structure
        StingerServer server(dataset.getMaxNumVertices(), args.alg_name);

        // Run the algorithm(s) after each inserted batch
        for (int64_t i = 0; i < dataset.batches.size(); ++i)
        {
            hooks.set_attr("batch", i);
            hooks.region_begin("preprocess");
            std::shared_ptr<DynoGraph::Batch> batch = dataset.getBatch(i);
            hooks.region_end();

            int64_t threshold = dataset.getTimestampForWindow(i);
            server.prepare(*batch, threshold);

            cerr << msg << "Running algorithms (pre-processing step)\n";
            server.updateAlgorithmsBeforeBatch();

            if (args.enable_deletions)
            {
                cerr << msg << "Deleting edges older than " << threshold << "\n";
                server.deleteOlderThan(threshold);
            }

            StingerServer::DegreeStats stats = server.compute_degree_distribution(*batch);
            hooks.set_stat("batch_degree_mean", stats.both.mean);
            hooks.set_stat("batch_degree_max", stats.both.max);
            hooks.set_stat("batch_degree_variance", stats.both.variance);
            hooks.set_stat("batch_degree_skew", stats.both.skew);
            hooks.set_stat("batch_num_vertices", batch->num_vertices_affected());

            cerr << msg << "Inserting batch " << i << "\n";
            server.insert(*batch);

            //TODO re-enable filtering at some point cerr << msg << "Filtering on >= " << threshold << "\n";
            cerr << msg << "Running algorithms (post-processing step)\n";
            server.updateAlgorithmsAfterBatch();

            // Clear out the graph between batches in snapshot mode
            if (args.sort_mode == DynoGraph::Args::SNAPSHOT)
            {
                // server = StingerServer() is no good here,
                // because this would allocate a new stinger before deallocating the old one.
                // We probably won't have enough memory for that.
                // Instead, use an explicit destructor call followed by placement new
                server.~StingerServer();
                new(&server) StingerServer(dataset.getMaxNumVertices(), args.alg_name);
            }
        }
    }

    return 0;
}
