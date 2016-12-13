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
    const std::string name;
    StingerAlgorithm(stinger_t * S, std::string name);

    // After onInit is called, there will be a pointer in stinger_registered_alg to alg_data
    // We can't allow this class to be copied or that pointer will be invalidated
    // But we still need move so we can use emplace_back
    StingerAlgorithm(const StingerAlgorithm& other) = delete;
    StingerAlgorithm(StingerAlgorithm&& other) = default;

    void observeInsertions(std::vector<stinger_edge_update> &recentInsertions);
    void observeDeletions(std::vector<stinger_edge_update> &recentDeletions);
    void observeVertexCount(int64_t nv);
    void setSources(const std::vector<int64_t> &sources);

    void onInit();
    void onPre();
    void onPost();
    static const std::vector<std::string> supported_algs;
};
