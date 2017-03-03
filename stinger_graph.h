#pragma once

#include <dynograph_util.h>

extern "C" {
#include <stinger_core/stinger.h>
}

struct StingerGraph
{
    stinger_t * S;

    StingerGraph(int64_t nv);
    ~StingerGraph();

    // Prevent copy
    StingerGraph(const StingerGraph& other)            = delete;
    StingerGraph& operator=(const StingerGraph& other) = delete;

    void insert_using_parallel_for_static_schedule(const DynoGraph::Batch &batch);
    void insert_using_parallel_for_dynamic_schedule(const DynoGraph::Batch &batch);
    void insert_using_set_initial_edges(const DynoGraph::Batch &batch);
    void insert_using_stinger_batch(const DynoGraph::Batch &batch);
    void deleteOlderThan(int64_t threshold);
    void printSize();
};
