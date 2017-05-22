#include "stinger_alg/dynamic_streaming_connected_components.h"
#include "stinger_net/stinger_alg_client.h"

using namespace gt::stinger;

int
main (int argc, char *argv[])
{
  StreamingConnectedComponents alg;
  run_alg_as_client(alg);
}
