#ifndef STINGER_DYNAMIC_CLUSTERING_H_
#define STINGER_DYNAMIC_CLUSTERING_H_

#include <stdint.h>
#include <string>
#include "stinger_core/stinger.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"
#include "stinger_utils/timer.h"
#include "stinger_alg/clustering.h"
#include "stinger_alg/streaming_algorithm.h"

namespace gt {
  namespace stinger {
    class ClusteringCoefficients : public IDynamicGraphAlgorithm
    {
    private:
      double * local_cc;
      int64_t * ntri;
      int64_t * affected;
    public:
        ClusteringCoefficients();
        ~ClusteringCoefficients();

        // Overridden from IDynamicGraphAlgorithm
        std::string getName();
        int64_t getDataPerVertex();
        std::string getDataDescription();
        void onInit(stinger_registered_alg * alg);
        void onPre(stinger_registered_alg * alg);
        void onPost(stinger_registered_alg * alg);
    };
  }
}

#endif