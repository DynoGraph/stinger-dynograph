#pragma once

#include "batch.h"
#include <memory>

namespace DynoGraph {

class IDataset
{
public:
    virtual int64_t getTimestampForWindow(int64_t batchId) const = 0;
    virtual std::shared_ptr<Batch> getBatch(int64_t batchId) = 0;
    virtual std::shared_ptr<Batch> getBatchesUpTo(int64_t batchId) = 0;
    virtual int64_t getNumBatches() const = 0;
    virtual int64_t getNumEdges() const = 0;
    virtual bool isDirected() const = 0;
    virtual int64_t getMaxVertexId() const = 0;
    virtual int64_t getMinTimestamp() const = 0;
    virtual int64_t getMaxTimestamp() const = 0;
    virtual void reset() {};
    virtual ~IDataset() = default;
};

} // end namespace DynoGraph