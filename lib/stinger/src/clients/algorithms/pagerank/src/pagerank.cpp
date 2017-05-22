#include <stdint.h>

extern "C" {
#include "stinger_alg/pagerank.h"
}
#include "stinger_alg/dynamic_pagerank.h"
#include "stinger_net/stinger_alg_client.h"

using namespace gt::stinger;

int
main(int argc, char *argv[])
{
  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Setup and register algorithm with the server
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  char name[1024];
  char type_str[1024];

  int type_specified = 0;
  int directed = 0;

  double epsilon = EPSILON_DEFAULT;
  double dampingfactor = DAMPINGFACTOR_DEFAULT;
  int64_t maxiter = MAXITER_DEFAULT;

  int opt = 0;
  while(-1 != (opt = getopt(argc, argv, "t:e:f:i:d?h"))) {
    switch(opt) {
      case 't': {
        snprintf(name, 1024, "pagerank_%s", optarg);
        strcpy(type_str,optarg);
        type_specified = 1;
      } break;
      case 'd': {
        directed = 1;
      } break;
      case 'e': {
        epsilon = atof(optarg);
      } break;
      case 'f': {
        dampingfactor = atof(optarg);
      } break;
      case 'i': {
        maxiter = atol(optarg);
      } break;
      default:
        printf("Unknown option '%c'\n", opt);
      case '?':
      case 'h': {
        printf(
          "PageRank\n"
          "==================================\n"
          "\n"
          "  -t <str>  Specify an edge type to run page rank over\n"
          "  -d        Use a PageRank that is safe on directed graphs\n"
          "  -e        Set PageRank Epsilon (default: %0.1e)\n"
          "  -f        Set PageRank Damping Factor (default: %lf)\n"
          "  -i        Set PageRank Max Iterations (default: %ld)\n"
          "\n",EPSILON_DEFAULT,DAMPINGFACTOR_DEFAULT,MAXITER_DEFAULT);
        return(opt);
      }
    }
  }

  PageRank alg(
    type_str,
    type_specified,
    directed,
    epsilon,
    dampingfactor,
    maxiter
  );
  run_alg_as_client(alg);
}
