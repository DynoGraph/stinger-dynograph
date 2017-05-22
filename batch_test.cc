#include "batch.h"
#include <gtest/gtest.h>

using namespace DynoGraph;

// Make sure we can filter a batch by timestamp
TEST(BatchTest, FilterBatch) {
    std::vector<Edge> edges = {
        {1, 2, 1, 100},
        {2, 3, 2, 200},
        {3, 4, 1, 300},
        {4, 5, 1, 400},
        {5, 6, 1, 500},
    };
    Batch batch(edges);
    ASSERT_EQ(batch.size(), 5);
    batch.filter(300);
    ASSERT_EQ(batch.size(), 3);
    ASSERT_EQ(batch[0], edges[2]);
    ASSERT_EQ(batch[1], edges[3]);
    ASSERT_EQ(batch[2], edges[4]);
}

// Make sure we can sort and dedup a batch by degree
TEST(BatchTest, DedupAndSortBatch) {
    std::vector<Edge> edges_before = {
        {1, 2, 1, 100},
        {2, 3, 1, 200},
        {2, 3, 1, 300},
        {2, 3, 1, 400},
        {2, 3, 1, 400},
        {2, 3, 1, 400},
        {2, 3, 1, 400},
        {2, 4, 1, 400},
        {3, 4, 1, 500},
        {3, 5, 1, 400},
        {3, 6, 1, 500},
        {3, 7, 1, 500},
        {3, 8, 1, 500},
        {3, 9, 1, 500},
    };

    std::vector<Edge> edges_after = {
        {3, 4, 1, 500},
        {3, 5, 1, 400},
        {3, 6, 1, 500},
        {3, 7, 1, 500},
        {3, 8, 1, 500},
        {3, 9, 1, 500},
        {2, 3, 1, 400},
        {2, 4, 1, 400},
        {1, 2, 1, 100},
    };
    Batch test_batch(edges_before);
    test_batch.dedup_and_sort_by_out_degree();
    Batch golden_batch(edges_after);

    auto batches_equal = [](const Batch& a, const Batch& b){
        if (a.size() != b.size()) {
            return ::testing::AssertionFailure();
        } else {
            bool equal = std::equal(a.begin(), a.end(), b.begin(),
                [](const Edge& x, const Edge& y) {
                    return x.src == y.src && x.dst == y.dst;
                }
            );
            return equal ? ::testing::AssertionSuccess() : ::testing::AssertionFailure();
        }
    };
    ASSERT_PRED2(batches_equal, test_batch, golden_batch);
}

TEST(BatchTest, GetMaxVertexId) {
    std::vector<Edge> edges = {
        {1, 2, 1, 100},
        {2, 3, 1, 200},
        {2, 3, 1, 300},
        {2, 3, 1, 400},
        {2, 3, 1, 400},
        {2, 3, 1, 400},
        {2, 10, 1, 400},
        {2, 4, 1, 400},
    };
    Batch batch(edges);
    ASSERT_EQ(batch.max_vertex_id(), 10);

    batch[2].src = 11;
    ASSERT_EQ(batch.max_vertex_id(), 11);
}

TEST(BatchTest, NumVerticesAffected) {
    std::vector<Edge> edges = {
        {1, 2, 1, 100},
        {2, 3, 1, 200},
        {2, 3, 1, 300},
        {2, 3, 1, 400},
        {2, 3, 1, 400},
        {2, 3, 1, 400},
        {2, 10, 1, 400},
        {2, 4, 1, 400},
    };
    Batch batch(edges);
    ASSERT_EQ(batch.num_vertices_affected(), 5);

    batch[2].src = 11;
    ASSERT_EQ(batch.num_vertices_affected(), 6);
}

