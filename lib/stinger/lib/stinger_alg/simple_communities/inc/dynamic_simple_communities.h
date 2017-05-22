#ifndef STINGER_DYNAMIC_SIMPLE_COMMUNITIES_H_
#define STINGER_DYNAMIC_SIMPLE_COMMUNITIES_H_

#include <stdint.h>
#include "stinger_net/stinger_alg.h"
#include "streaming_algorithm.h"
extern "C" {
#include "defs.h"
#include "graph-el.h"
#include "community.h"
#include "community-update.h"
}
namespace gt {
  namespace stinger {
    class SimpleCommunities : public IDynamicGraphAlgorithm
    {
    private:
        int64_t iter;
        community_state cstate;
        int64_t * __restrict__ cmap;
    public:
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