
// Adapted from the stinger project <https://github.com/stingergraph/stinger> rmat.c
#include <cinttypes>
#include <random>
#include "prng_engine.hpp"

namespace DynoGraph {

/*
 * RMAT edge generator
 * Implements discard() to skip ahead in the random stream in constant time
 */
class rmat_edge_generator {
private:
/*
 * RANDOM NUMBER GENERATION
 */
    // Generates uniformly distributed uint32_t's
    // Implements discard() in constant time
    typedef sitmo::prng_engine prng_engine;
    //typedef NotRandomEngine prng_engine;
    prng_engine rng_engine;
    // Converts values from the rng_engine into double type
    std::uniform_real_distribution<double> rng_distribution;
    // Helper function to get a number from the distribution
    double rng() { return rng_distribution(rng_engine); }
    /* Stores the number of times std::uniform_real_distribution<double> will
     * call the generator for each number generated
     */
    size_t k;
    size_t getk() const {
        /*
         * From http://en.cppreference.com/w/cpp/numeric/random/generate_canonical
         * To generate enough entropy, generate_canonical() will call the generator g() exactly k times,
         * where k = max(1, ceil(b / log2(R)))
         * and b = std::min<std::size_t>(bits, std::numeric_limits<RealType>::digits)
         * and R = g.max() - g.min() + 1.
         *
         * In our case,
         * bits = std::numeric_limits<prng_engine::result_type>::digits
         * and RealType = double
         * and __urng = prng_engine
         */
        typedef double _RealType;
        size_t __bits = std::numeric_limits<_RealType>::digits;
        const prng_engine& __urng = rng_engine;
        /*
         * Remaining code copied from std::generate_canonical(), defined in bits/random.tcc
         * Even if that implementation changes, the equation should still be the same
         */
        const size_t __b
                = std::min(static_cast<size_t>(std::numeric_limits<_RealType>::digits),
                        __bits);
        const long double __r = static_cast<long double>(__urng.max())
                                - static_cast<long double>(__urng.min()) + 1.0L;
        const size_t __log2r = std::log(__r) / std::log(2.0L);
        size_t __k = std::max<size_t>(1UL, (__b + __log2r - 1UL) / __log2r);

        return __k;
    }

/*
 * RMAT PARAMETERS
 */
    // log2(num_vertices)
    int64_t SCALE;
    // RMAT parameters
    double a, b, c, d;

public:
    rmat_edge_generator(int64_t nv, double a, double b, double c, double d, uint32_t seed=0)
    : rng_engine(seed)
    , rng_distribution(0, 1)
    , k(getk())
    , SCALE(0), a(a), b(b), c(c), d(d)
    {
        while (nv >>= 1) { ++SCALE; }
    }

    // Skips past the next n randomly generated edges
    void discard(uint64_t n)
    {
        // The loop in next_edge iterates SCALE-1 times, using 5 random numbers in each iteration
        // The final iteration before the break uses one more random number
        n *= 5 * (SCALE-1) + 1;
        // std::uniform_real_distribution<double> calls the generator multiple times to get enough entropy
        n *= k;

        rng_engine.discard(n);
    }

    void next_edge(int64_t *src, int64_t *dst)
    {
        double A = a;
        double B = b;
        double C = c;
        double D = d;
        int64_t i = 0, j = 0;
        int64_t bit = ((int64_t) 1) << (SCALE - 1);

        while (1) {
            const double r = rng();
            if (r > A) {                /* outside quadrant 1 */
                if (r <= A + B)           /* in quadrant 2 */
                    j |= bit;
                else if (r <= A + B + C)  /* in quadrant 3 */
                    i |= bit;
                else {                    /* in quadrant 4 */
                    j |= bit;
                    i |= bit;
                }
            }
            if (1 == bit)
                break;

            /*
              Assuming R is in (0, 1), 0.95 + 0.1 * R is in (0.95, 1.05).
              So the new probabilities are *not* the old +/- 10% but
              instead the old +/- 5%.
            */
            A *= (9.5 + rng()) / 10;
            B *= (9.5 + rng()) / 10;
            C *= (9.5 + rng()) / 10;
            D *= (9.5 + rng()) / 10;
            /* Used 5 random numbers. */

            {
                const double norm = 1.0 / (A + B + C + D);
                A *= norm;
                B *= norm;
                C *= norm;
            }
            /* So long as +/- are monotonic, ensure a+b+c+d <= 1.0 */
            D = 1.0 - (A + B + C);

            bit >>= 1;
        }
        /* Iterates SCALE times. */
        *src = i;
        *dst = j;
    }
};

} // end namespace DynoGraph
