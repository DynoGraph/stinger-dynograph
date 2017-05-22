#ifndef STINGER_DYNAMIC_PAGERANK_H_
#define STINGER_DYNAMIC_PAGERANK_H_

#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include "stinger_net/stinger_alg.h"
#include "streaming_algorithm.h"
namespace gt {
  namespace stinger {
    class PageRank : public IDynamicGraphAlgorithm
    {
    private:
        const char *type_str;
        int type_specified;
        int directed;
        double epsilon;
        double dampingfactor;
        int64_t maxiter;

        double * pr;
        double * tmp_pr;
    public:
        PageRank(
          const char * type_str,
          int type_specified,
          int directed,
          double epsilon,
          double dampingfactor,
          int64_t maxiter);

        ~PageRank();

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