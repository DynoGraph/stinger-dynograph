#include "logger.h"
#include "rmat_dataset.h"
#include <iostream>
#include <stdio.h>

using namespace DynoGraph;

using std::cerr;

Logger &logger = DynoGraph::Logger::get_instance();

void print_help_and_quit()
{
    logger << "Usage: ./rmat_dataset_dump <rmat_args>\n";
    die();
}

int main(int argc, const char* argv[])
{
    if (argc != 2) { print_help_and_quit(); }

    // Open output file
    std::string filename = argv[1];
    FILE* fp = fopen(filename.c_str(), "wb");
    if (fp == NULL) {
        logger << "Cannot open " << filename << "\n";
        die();
    }

    // Parse rmat arguments
    RmatArgs rmat_args = RmatArgs::from_string(argv[1]);
    std::string error = rmat_args.validate();
    if (!error.empty())
    {
        logger << error << "\n";
        print_help_and_quit();
    }

    // Create the rmat graph generator
    rmat_edge_generator generator(
        rmat_args.num_vertices,
        rmat_args.a,
        rmat_args.b,
        rmat_args.c,
        rmat_args.d,
        0
    );

    // Generate the edge list
    logger << "Generating RMAT graph with "
           << rmat_args.num_edges << " edges and "
           << rmat_args.num_vertices << " vertices.\n";
    RmatBatch edge_list(generator, rmat_args.num_edges, 0);

    // Dump to file
    fwrite(edge_list.begin(), sizeof(Edge), edge_list.size(), fp);

    // Clean up
    fclose(fp);
    logger << "...Done\n";

    return 0;
}