#ifndef STINGER_DYNAMIC_BFS_H_
#define STINGER_DYNAMIC_BFS_H_

#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <vector>
#include "stinger_net/stinger_alg.h"
#include "streaming_algorithm.h"
namespace gt {
  namespace stinger {
    class BreadthFirstSearch : public IDynamicGraphAlgorithm
    {
    private:
        int64_t *level;
        std::vector<int64_t> sources;
    public:
        BreadthFirstSearch();

        void setSources(const std::vector<int64_t> &sources);

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