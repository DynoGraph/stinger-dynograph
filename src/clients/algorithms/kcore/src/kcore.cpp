#include "stinger_alg/dynamic_kcore.h"
#include "stinger_net/stinger_alg_client.h"

using namespace gt::stinger;

int
main(int argc, char *argv[])
{
    KCore alg;
    run_alg_as_client(alg);
}
