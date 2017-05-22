#include "stinger_net/stinger_alg_client.h"
#include "stinger_alg/dynamic_clustering.h"

using namespace gt::stinger;

int
main(int argc, char *argv[])
{
  ClusteringCoefficients alg;
  run_alg_as_client(alg);
}
