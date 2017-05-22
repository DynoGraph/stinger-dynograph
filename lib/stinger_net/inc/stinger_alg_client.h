#ifndef STINGER_ALGORITHM_CLIENT_H_
#define STINGER_ALGORITHM_CLIENT_H_
#include "streaming_algorithm.h"
#include "stinger_net/stinger_alg.h"

namespace gt {
  namespace stinger {
    void run_alg_as_client(IDynamicGraphAlgorithm &alg);
  }
}

#endif