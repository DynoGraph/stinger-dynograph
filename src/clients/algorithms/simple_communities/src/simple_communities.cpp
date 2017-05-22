#include "stinger_alg/dynamic_simple_communities.h"
#include "stinger_net/stinger_alg_client.h"

using namespace gt::stinger;

int
main(int argc, char *argv[])
{
  SimpleCommunities alg;
  run_alg_as_client(alg);
  return 0;
}
