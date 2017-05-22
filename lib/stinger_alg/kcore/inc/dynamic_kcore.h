#ifndef STINGER_DYNAMIC_KCORE_H_
#define STINGER_DYNAMIC_KCORE_H_

#include <stdint.h>
#include "stinger_net/stinger_alg.h"
#include "streaming_algorithm.h"
namespace gt {
  namespace stinger {
    class KCore : public IDynamicGraphAlgorithm
    {
    private:
        int64_t * kcore;
        int64_t * count;
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