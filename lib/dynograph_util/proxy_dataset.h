#pragma once

#include "idataset.h"
#include <memory>

namespace DynoGraph {

//
class ProxyDataset : public IDataset {
private:
    std::shared_ptr<IDataset> impl;
public:
    ProxyDataset(std::shared_ptr<IDataset> dataset);
    int64_t getTimestampForWindow(int64_t batchId) const;
    std::shared_ptr<Batch> getBatch(int64_t batchId);
    std::shared_ptr<Batch> getBatchesUpTo(int64_t batchId);
    int64_t getNumBatches() const;
    int64_t getNumEdges() const;
    bool isDirected() const;
    int64_t getMaxVertexId() const;
    int64_t getMinTimestamp() const;
    int64_t getMaxTimestamp() const;
    void reset();
};

}
