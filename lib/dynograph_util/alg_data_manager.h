#pragma once

#include <map>
#include <string>
#include <vector>
#include <cinttypes>
#include "pvector.h"
#include "range.h"

namespace DynoGraph {

class AlgDataManager
{
private:
    std::map<std::string, pvector<int64_t>> last_epoch_data;
    std::map<std::string, pvector<int64_t>> current_epoch_data;
    std::string path;
public:
    AlgDataManager(int64_t nv, std::vector<std::string> alg_names);
    void next_epoch();
    void rollback();
    void dump(int64_t epoch) const;
    DynoGraph::Range<int64_t> get_data_for_alg(std::string alg_name);
};

} // end namespace DynoGraph
