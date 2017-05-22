#pragma once

// Provides a test template for implementations of DynoGraph::DynamicGraph

#include <hooks.h>
#include "edgelist_dataset.h"
#include <gtest/gtest.h>
#include "reference_impl.h"

using namespace DynoGraph;

// Test fixture, to be templated with a class that implements DynamicGraph
template <typename graph_t>
class ImplTest : public ::testing::Test {
private:
    graph_t graph;
protected:
    DynamicGraph &impl;
public:
    ImplTest()
    : graph({1, "dummy", 3, graph_t::get_supported_algs(), Args::SORT_MODE::UNSORTED, 1.0, 1}, 1000)
    , impl(graph)
    {
        // Initialize edge count
        Hooks::getInstance();
    }
};
TYPED_TEST_CASE_P(ImplTest);

// Make sure it can run all the algs it advertises
TYPED_TEST_P(ImplTest, CheckAlgs)
{
    // Create a simple graph
    std::vector<Edge> edges = {
        {1, 2, 1, 100},
        {2, 3, 2, 200},
        {3, 1, 1, 300},
    };
    Batch batch(edges.begin(), edges.end());
    this->impl.insert_batch(batch);

    // Allocate data for alg
    std::vector<int64_t> alg_data(1001);

    // Run all supported algs
    for (auto alg_name : this->impl.args.alg_names)
    {
        this->impl.update_alg(alg_name, {1}, alg_data);
    }
    EXPECT_EQ(this->impl.get_num_edges(), 3);
};

// Make sure empty graph is really empty
TYPED_TEST_P(ImplTest, EmptyGraph)
{
    EXPECT_EQ(this->impl.get_num_vertices(), 0);
    EXPECT_EQ(this->impl.get_num_edges(), 0);
};

// Insert several duplicates of the same edge and make sure num_edges == 1
TYPED_TEST_P(ImplTest, DuplicateEdgeInsert)
{
    // Create a batch with some duplicate edges
    std::vector<Edge> edges = {
        {2, 3, 0, 0},
        {2, 3, 0, 0},
        {2, 3, 0, 0},
    };
    Batch batch(edges.begin(), edges.end());

    // Ask the impl to insert the edges
    this->impl.insert_batch(batch);

    // Make sure it reports the right number of edges back
    EXPECT_EQ(this->impl.get_num_edges(), 1);

    // Insert the duplicates again, just to make sure the deduplication happens no matter what
    this->impl.insert_batch(batch);
    EXPECT_EQ(this->impl.get_num_edges(), 1);

}

// Insert a bidirectional edge between two nodes and make sure it is counted as two edges
TYPED_TEST_P(ImplTest, BidirectionalEdgeInsert)
{
    // Create two edges going both directions
    std::vector<Edge> edges = {
        {2, 3, 0, 0},
        {3, 2, 0, 0},
        {1, 2, 0, 0},
    };
    Batch batch(edges.begin(), edges.end());

    // Ask the impl to insert the batch
    this->impl.insert_batch(batch);

    // Make sure it reports the right number of edges back
    EXPECT_EQ(this->impl.get_num_edges(), 3);
}

// Make sure we get the right value back from get_out_degree
TYPED_TEST_P(ImplTest, GetOutDegree)
{
    // Create several edges emanating from the same source vertex
    std::vector<Edge> edges = {
        {1, 3, 0, 0},
        {1, 4, 0, 0},
        {1, 5, 0, 0},
    };
    Batch batch(edges.begin(), edges.end());

    // Ask the impl to insert the edge
    this->impl.insert_batch(batch);

    // Make sure it reports the right number of edges back
    EXPECT_EQ(this->impl.get_out_degree(1), 3);
}

// Make sure the graph can actually delete edges
TYPED_TEST_P(ImplTest, DeleteOlderThan)
{
    // Create several edges with distinct timestamps
    std::vector<Edge> edges = {
        {1, 3, 0, 100},
        {1, 4, 0, 200},
        {1, 5, 0, 300},
    };
    Batch batch(edges.begin(), edges.end());
    this->impl.insert_batch(batch);
    EXPECT_EQ(this->impl.get_num_edges(), 3);
    EXPECT_EQ(this->impl.get_out_degree(1), 3);

    // Should delete 1->3, leaving two edges
    this->impl.delete_edges_older_than(200);
    EXPECT_EQ(this->impl.get_num_edges(), 2);
    EXPECT_EQ(this->impl.get_out_degree(1), 2);

    // Shouldn't delete anything
    this->impl.delete_edges_older_than(0);
    EXPECT_EQ(this->impl.get_num_edges(), 2);
    EXPECT_EQ(this->impl.get_out_degree(1), 2);

    // Should delete 1->4 and 1->5, leaving the graph empty
    this->impl.delete_edges_older_than(400);
    EXPECT_EQ(this->impl.get_num_edges(), 0);
    EXPECT_EQ(this->impl.get_out_degree(1), 0);
}

// Make sure timestamps are updated on insert
TYPED_TEST_P(ImplTest, TimestampUpdate)
{
    // Create several edges with distinct timestamps
    std::vector<Edge> edges1 = {
        {1, 3, 0, 100},
        {1, 4, 0, 200},
        {1, 5, 0, 300},
    };
    Batch batch1(edges1.begin(), edges1.end());
    this->impl.insert_batch(batch1);
    EXPECT_EQ(this->impl.get_num_edges(), 3);
    EXPECT_EQ(this->impl.get_out_degree(1), 3);

    // Insert identical edges with updated timestamps
    std::vector<Edge> edges2 = {
        {1, 3, 0, 400},
        {1, 4, 0, 500},
        {1, 5, 0, 600},
    };
    Batch batch2(edges2.begin(), edges2.end());
    this->impl.insert_batch(batch2);

    // Should delete 1->3, leaving two edges
    this->impl.delete_edges_older_than(500);
    EXPECT_EQ(this->impl.get_num_edges(), 2);
    EXPECT_EQ(this->impl.get_out_degree(1), 2);
}

// Make sure it can pick the highest degree vertices
TYPED_TEST_P(ImplTest, PickSourceVertex)
{
    // Create a test graph
    // vertex 2 has degree 4
    // vertex 3 and 1 have degree 3, so 1 should be chosen
    std::vector<Edge> edges1 = {
        {9, 3, 0, 100},
        {9, 4, 0, 200},
        {9, 5, 0, 300},
        {2, 3, 0, 400},
        {2, 4, 0, 500},
        {2, 5, 0, 500},
        {2, 6, 0, 600},
        {3, 7, 0, 700},
        {3, 8, 0, 800},
        {3, 9, 0, 900}
    };
    Batch batch1(edges1.begin(), edges1.end());
    this->impl.insert_batch(batch1);
    EXPECT_EQ(this->impl.get_num_edges(), edges1.size());

    std::vector<int64_t> vertices;
    vertices = this->impl.get_high_degree_vertices(0);
    EXPECT_EQ(vertices.size(), 0);

    vertices = this->impl.get_high_degree_vertices(1);
    EXPECT_EQ(vertices.size(), 1);
    EXPECT_EQ(vertices[0], 2);

    vertices = this->impl.get_high_degree_vertices(2);
    EXPECT_EQ(vertices.size(), 2);
    EXPECT_TRUE(std::find(vertices.begin(), vertices.end(), 2) != vertices.end());
    EXPECT_TRUE(std::find(vertices.begin(), vertices.end(), 3) != vertices.end());

}

// All tests in this file must be registered here
REGISTER_TYPED_TEST_CASE_P( ImplTest
    ,CheckAlgs
    ,EmptyGraph
    ,DuplicateEdgeInsert
    ,BidirectionalEdgeInsert
    ,GetOutDegree
    ,DeleteOlderThan
    ,TimestampUpdate
    ,PickSourceVertex
);

template <typename graph_t>
class CompareWithReferenceTest : public ::testing::Test {
protected:
    Args args;
    EdgeListDataset dataset;
private:
    graph_t test_graph;
    reference_impl ref_graph;
protected:
    DynamicGraph &test_impl;
    DynamicGraph &ref_impl;
public:
    CompareWithReferenceTest()
    : args{1, "dynograph_util/data/worldcup-10K.graph.bin", 500, {}, Args::SORT_MODE::UNSORTED, 0.7, 1}
    , dataset(args)
    , test_graph(args, dataset.getMaxVertexId())
    , ref_graph(args, dataset.getMaxVertexId())
    , test_impl(test_graph)
    , ref_impl(ref_graph)
    {

    }
};
TYPED_TEST_CASE_P(CompareWithReferenceTest);

TYPED_TEST_P(CompareWithReferenceTest, MatchGraphState)
{
    for (uint64_t batch = 0; batch < this->dataset.getNumBatches(); ++batch)
    {
        auto b = this->dataset.getBatch(batch);
        this->test_impl.insert_batch(*b);
        this->ref_impl.insert_batch(*b);

        ASSERT_EQ(
            this->test_impl.get_num_edges(),
            this->ref_impl.get_num_edges()
        );
        ASSERT_EQ(
            this->test_impl.get_high_degree_vertices(1)[0],
            this->ref_impl.get_high_degree_vertices(1)[0]
        );
        int64_t nv = this->test_impl.get_num_vertices();
        for (int64_t v = 0; v < nv; ++v)
        {
            ASSERT_EQ(
                this->test_impl.get_out_degree(v),
                this->ref_impl.get_out_degree(v)
            );
        }
    }
}

REGISTER_TYPED_TEST_CASE_P( CompareWithReferenceTest , MatchGraphState);

// To instantiate this test, create a source file that includes this header and these lines:
//      INSTANTIATE_TYPED_TEST_CASE_P(TEST_NAME_HERE, ImplTest, ClassThatImplementsDynamicGraph);
//      INSTANTIATE_TYPED_TEST_CASE_P(TEST_NAME_HERE, CompareWithReferenceTest, ClassThatImplementsDynamicGraph);
// Then link with gtest_main
