#include "stinger_alg/dynamic_simple_communities_updating.h"
#include "stinger_net/stinger_alg_client.h"

using namespace gt::stinger;

int
main(int argc, char *argv[])
{
  int initial_compute = 0;
  if (getenv ("COMPUTE"))
    initial_compute = 1;
  for (int k = 1; k < argc; ++k) {
    if (0 == strcmp(argv[k], "--compute"))
      initial_compute = 1;
    else if (0 == strcmp(argv[k], "--help") || 0 == strcmp(argv[1], "-h")) {
      fprintf (stderr, "Pass --compute or set COMPUTE env. var to compute\n"
              "an initial community map");
    }
  }

  SimpleCommunitiesUpdating alg(initial_compute);
  run_alg_as_client(alg);
  return 0;
}
