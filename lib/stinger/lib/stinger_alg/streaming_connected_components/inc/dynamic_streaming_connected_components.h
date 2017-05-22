#ifndef STINGER_DYNAMIC_STREAMING_CONNECTED_COMPONENTS_H_
#define STINGER_DYNAMIC_STREAMING_CONNECTED_COMPONENTS_H_

#include <stdint.h>
#include "stinger_net/stinger_alg.h"
#include "streaming_algorithm.h"
extern "C" {
#include "stinger_alg/streaming_connected_components.h"
}
namespace gt {
  namespace stinger {
    class StreamingConnectedComponents : public IDynamicGraphAlgorithm
    {
    private:
        int64_t * components;
        int64_t * component_size;
        stinger_scc_internal scc_internal;
        stinger_connected_components_stats stats;
    public:
        ~StreamingConnectedComponents();
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