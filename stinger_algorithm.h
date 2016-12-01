#pragma once

#include <vector>
#include <memory>
#include <string>
#include <stdint.h>

#include <stinger_alg/streaming_algorithm.h>
#include <stinger_net/stinger_alg.h>

class StingerAlgorithm
{
protected:
    std::shared_ptr<gt::stinger::IDynamicGraphAlgorithm> impl;
    stinger_registered_alg data;
    std::vector<uint8_t> alg_data;
public:
    StingerAlgorithm(stinger_t * S, std::string name);

    void observeInsertions(std::vector<stinger_edge_update> &recentInsertions);
    void observeDeletions(std::vector<stinger_edge_update> &recentDeletions);
    void observeVertexCount(int64_t nv);
    void pickSources();
    std::vector<int64_t> find_high_degree_vertices(int64_t num);

    void onInit();
    void onPre();
    void onPost();
    std::string name() const;
};
