#include <stdint.h>
#include <unistd.h>

#include "stinger_alg/dynamic_betweenness.h"
#include "stinger_net/stinger_alg_client.h"

using namespace gt::stinger;

int
main(int argc, char *argv[])
{
    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
    * Options and arg parsing
    * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    int64_t num_samples = 256;
    double weighting = 0.5;
    uint8_t do_weighted = 1;
    const char * alg_name = "betweenness_centrality";

    int opt = 0;
    while(-1 != (opt = getopt(argc, argv, "w:s:n:x?h"))) {
        switch(opt) {
            case 'w': {
                weighting = atof(optarg);
                if(weighting > 1.0) {
                    weighting = 1.0;
                } else if(weighting < 0.0) {
                    weighting = 0.0;
                }
            } break;

            case 's': {
                num_samples = atol(optarg);
                if(num_samples < 1) {
                    num_samples = 1;
                }
            } break;

            case 'n': {
                alg_name = optarg;
            } break;

            case 'x': {
                do_weighted = 0;
            } break;

            default:
                printf("Unknown option '%c'\n", opt);
            case '?':
            case 'h': {
                printf(
                    "Approximate Betweenness Centrality\n"
                    "==================================\n"
                    "\n"
                    "Measures an approximate BC on the STINGER graph by randomly sampling n vertices each pass and\n"
                    "performing one breadth-first traversal from the Brandes algorithm. The samples are then\n"
                    "aggregated and potentially weighted with the result from\n"
                    "the last pass\n"
                    "\n"
                    "  -s <num>  Set the number of samples (%ld by default)\n"
                    "  -w <num>  Set the weighintg (0.0 - 1.0) (%lf by default)\n"
                    "  -x        Disable weighting\n"
                    "  -n <str>  Set the algorithm name (%s by default)\n"
                    "\n", num_samples, weighting, alg_name
                );
                return(opt);
            }
        }
    }

    // Run the algorithm, updating with each new batch
    BetweennessCentrality alg(num_samples, weighting, do_weighted);
    run_alg_as_client(alg);

    return 0;
}
