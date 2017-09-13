#ifndef STINGER_DYNAMIC_GRAPH_ALGORITHM_H_
#define STINGER_DYNAMIC_GRAPH_ALGORITHM_H_

#include <string>
#include <stdint.h>
#include <stinger_net/stinger_alg.h>
namespace gt {
  namespace stinger {
    class IDynamicGraphAlgorithm
    {
    public:
        // Return the name of the algorithm
        virtual std::string getName() = 0;
        // How much storage does this algorithm require per vertex?
        virtual int64_t getDataPerVertex() = 0;
        // Description of algorithm data, see stinger_alg.h for format
        virtual std::string getDataDescription() = 0;

        // Called once at startup
        virtual void onInit(stinger_registered_alg * alg) = 0;
        // Called before each batch
        virtual void onPre(stinger_registered_alg * alg) = 0;
        // Called after each batch
        virtual void onPost(stinger_registered_alg * alg) = 0;

        // Allows base class destructor to be called polymorphically
        virtual ~IDynamicGraphAlgorithm() {}
    };
  }
}
#endif