#include "stinger_server.h"
#include <dynograph_impl_test.h>

INSTANTIATE_TYPED_TEST_CASE_P(STINGER_DYNOGRAPH, ImplTest, StingerServer);
INSTANTIATE_TYPED_TEST_CASE_P(STINGER_DYNOGRAPH, CompareWithReferenceTest, StingerServer);

TEST(STINGER_DYNOGRAPH, SetInitialEdgesTest)
{
    DynoGraph::Args args = {1, "dynograph_util/data/worldcup-10K.graph.bin", 10, {}, Args::SORT_MODE::SNAPSHOT, 1.0, 1};
    DynoGraph::Dataset dataset(args);
    auto batch = dataset.getBatch(0);


    int64_t nv = dataset.getMaxVertexId() + 1;
    StingerGraph graph(nv);
    graph.insert_using_set_initial_edges(*batch);

    reference_impl golden(args, nv);
    golden.insert_batch(*batch);

    ASSERT_EQ(
        golden.get_num_edges(),
        stinger_edges_up_to(graph.S, nv)
    );
    for (int64_t v = 0; v < nv; ++v)
    {
        ASSERT_EQ(
            golden.get_out_degree(v),
            stinger_outdegree_get(graph.S, v)
        );
    }

}