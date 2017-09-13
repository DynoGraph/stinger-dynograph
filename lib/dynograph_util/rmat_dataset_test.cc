
// Provides unit tests for the RmatDataset class

#include "reference_impl.h"
#include "rmat_dataset.h"
#include <gtest/gtest.h>

using namespace DynoGraph;

TEST(RmatArgsTest, ParseFromString)
{
    RmatArgs rmat_args = RmatArgs::from_string("0.55-0.20-0.10-0.15-12G-8M.rmat");
    EXPECT_EQ(rmat_args.a, 0.55);
    EXPECT_EQ(rmat_args.b, 0.20);
    EXPECT_EQ(rmat_args.c, 0.10);
    EXPECT_EQ(rmat_args.d, 0.15);
    EXPECT_EQ(rmat_args.num_edges, 12LL * 1024 * 1024 * 1024);
    EXPECT_EQ(rmat_args.num_vertices, 8LL * 1024 * 1024);
}

TEST(RmatArgsTest, Validation)
{
    RmatArgs rmat_args;
    rmat_args = RmatArgs::from_string("0.55-0.20-0.10-0.15-12G-8M.rmat");
    EXPECT_TRUE(rmat_args.validate().empty());
    // Values don't sum to one
    rmat_args = RmatArgs::from_string("0.35-0.20-0.10-0.15-12G-8M.rmat");
    EXPECT_FALSE(rmat_args.validate().empty());
}

TEST(PrngEngineTest, DiscardActuallyWorks)
{
    sitmo::prng_engine engine_a, engine_b;
    std::uniform_real_distribution<double> dist_a, dist_b;

    auto a = [&]() { return dist_a(engine_a); };
    auto b = [&]() { return dist_b(engine_b); };

    ASSERT_EQ(a(), b());
    ASSERT_EQ(a(), b());

    // Make sure skipping does the right thing
    for (int skip : {1, 5, 1000}) {
        // Manually advance a
        for (int i = 0; i < skip; ++i) { a(); }
        // Use "discard" to advance b
        // 2 is the number of times std::uniform_real_distribution<double>
        // will call sitmo::prng_engine
        engine_b.discard(skip*2);

        // Make sure they have the same state
        EXPECT_EQ(a(), b());
        EXPECT_EQ(a(), b());
    }
}

TEST(RmatEdgeGeneratorTest, DiscardActuallyWorks)
{
    // Create an rmat edge generator
    RmatArgs rmat_args = RmatArgs::from_string("0.55-0.20-0.10-0.15-12G-8M.rmat");
    uint32_t seed = 0;
    rmat_edge_generator gen1(
        rmat_args.num_vertices,
        rmat_args.a,
        rmat_args.b,
        rmat_args.c,
        rmat_args.d,
        seed
    );

    // Make a copy
    rmat_edge_generator gen2 = gen1;

    // Helper func to get the next edge from a generator (init weight and timestamp to 0)
    auto next = [](rmat_edge_generator& gen) {
        Edge e;
        gen.next_edge(&e.src, &e.dst);
        e.weight = 0;
        e.timestamp = 0;
        return e;
    };

    // Make sure the copy has the same state
    ASSERT_EQ(next(gen1), next(gen2));
    ASSERT_EQ(next(gen1), next(gen2));

    // Make sure skipping does the right thing
    for (int skip : {1, 5, 1000}) {
        // Manually advance gen1
        for (int i = 0; i < skip; ++i) { next(gen1); }
        // Use "discard" to advance gen2
        gen2.discard(skip);
        // Make sure they have the same state
        ASSERT_EQ(next(gen1), next(gen2));
        ASSERT_EQ(next(gen1), next(gen2));
    }

}

#if defined(_OPENMP)
#include <omp.h>

TEST(RmatDatasetTest, DeterministicParallelGeneration)
{
    RmatArgs rmat_args = RmatArgs::from_string("0.55-0.20-0.10-0.15-25K-10K.rmat");
    int64_t num_batches = 10;
    int64_t batch_size = rmat_args.num_edges / num_batches;
    Args args = {1, "dummy", num_batches, {}, DynoGraph::Args::SORT_MODE::UNSORTED, 1.0, 1, 1};

    using std::shared_ptr;
    using std::vector;
    vector<shared_ptr<Batch>> serial_batches;
    vector<shared_ptr<Batch>> parallel_batches;
    // Initialize the RMAT graph generator
    RmatDataset dataset(args, rmat_args);
    // Remember how many threads we have
    int max_num_threads = omp_get_max_threads();
    // Generate all the batches serially
    omp_set_num_threads(1);
    for (int64_t batch_id = 0; batch_id < num_batches; ++batch_id) {
        serial_batches.push_back(dataset.getBatch(batch_id));
    }
    // Reset the generator
    dataset.reset();
    // Generate all the batches in parallel
    omp_set_num_threads(max_num_threads);
    for (int64_t batch_id = 0; batch_id < num_batches; ++batch_id) {
        parallel_batches.push_back(dataset.getBatch(batch_id));
    }

    auto batches_equal = [](const Batch& a, const Batch& b){
        if (a.size() != b.size()) {
            return ::testing::AssertionFailure();
        } else {
            return std::equal(a.begin(), a.end(), b.begin())
                ? ::testing::AssertionSuccess() : ::testing::AssertionFailure();
        }
    };

    // Compare serial/parallel to make sure they match
    for (int64_t batch_id = 0; batch_id < num_batches; ++batch_id) {
        ASSERT_TRUE(batches_equal(*serial_batches[batch_id], *parallel_batches[batch_id]));
    }
}
#endif

TEST(RmatDatasetTest, NoSelfEdges)
{
    Args args = {1, "dummy", 1000, {}, DynoGraph::Args::SORT_MODE::SNAPSHOT, 1.0, 1, 1};
    RmatArgs rmat_args = RmatArgs::from_string("0.55-0.20-0.10-0.15-10K-10K.rmat");
    RmatDataset dataset(args, rmat_args);

    int64_t num_batches = dataset.getNumBatches();

    for (int64_t batch_id = 0; batch_id < num_batches; ++batch_id)
    {
        std::shared_ptr<Batch> batch = dataset.getBatch(batch_id);
        EXPECT_EQ(batch->size(), args.batch_size);
        for (Edge e : *batch) {
            EXPECT_NE(e.src, e.dst);
        }
    }
}