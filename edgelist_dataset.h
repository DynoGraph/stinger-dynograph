#pragma once
#include <memory>
#include "args.h"
#include "batch.h"
#include "idataset.h"
#include "pvector.h"

namespace DynoGraph {

class EdgeListDataset : public IDataset
{
private:
    void loadEdgesBinary(std::string path);
    void loadEdgesAscii(std::string path);

    Args args;
    bool directed;
    int64_t max_vertex_id;
    int64_t min_timestamp;
    int64_t max_timestamp;

    pvector<Edge> edges;
    pvector<Batch> batches;

public:
    EdgeListDataset(Args args);

    int64_t getTimestampForWindow(int64_t batchId) const;
    std::shared_ptr<Batch> getBatch(int64_t batchId);
    std::shared_ptr<Batch> getBatchesUpTo(int64_t batchId);
    int64_t getNumBatches() const;
    int64_t getNumEdges() const;
    int64_t getMinTimestamp() const;
    int64_t getMaxTimestamp() const;

    bool isDirected() const;
    int64_t getMaxVertexId() const;
};

} // end namespace DynoGraph
