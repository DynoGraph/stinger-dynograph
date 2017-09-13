#include "proxy_dataset.h"
#include "mpi_macros.h"
#include "helpers.h"

using namespace DynoGraph;
using std::shared_ptr;
using std::make_shared;

ProxyDataset::ProxyDataset(shared_ptr<IDataset> dataset) : impl(dataset) {}

int64_t
ProxyDataset::getTimestampForWindow(int64_t batchId) const
{
    int64_t timestamp;
    MPI_RANK_0_ONLY { timestamp = impl->getTimestampForWindow(batchId); }
    MPI_BROADCAST_RESULT(timestamp);
    return timestamp;
};

shared_ptr<Batch>
ProxyDataset::getBatch(int64_t batchId)
{
    MPI_RANK_0_ONLY {
        return impl->getBatch(batchId);
    } else {
        // For MPI, ranks other than zero get an empty batch
        return make_shared<Batch>();
    }
}

shared_ptr<Batch>
ProxyDataset::getBatchesUpTo(int64_t batchId)
{
    MPI_RANK_0_ONLY {
        return impl->getBatchesUpTo(batchId);
    } else {
        // For MPI, ranks other than zero get an empty batch
        return make_shared<Batch>();
    }

}

bool
ProxyDataset::isDirected() const
{
    bool retval;
    MPI_RANK_0_ONLY { retval = impl->isDirected(); }
    MPI_BROADCAST_RESULT(retval);
    return retval;
}

int64_t
ProxyDataset::getMaxVertexId() const
{
    int64_t retval;
    MPI_RANK_0_ONLY { retval = impl->getMaxVertexId(); }
    MPI_BROADCAST_RESULT(retval);
    return retval;
}

int64_t
ProxyDataset::getNumBatches() const {
    int64_t retval;
    MPI_RANK_0_ONLY { retval = impl->getNumBatches(); }
    MPI_BROADCAST_RESULT(retval);
    return retval;
};

int64_t
ProxyDataset::getNumEdges() const {
    int64_t retval;
    MPI_RANK_0_ONLY { retval = impl->getNumEdges(); }
    MPI_BROADCAST_RESULT(retval);
    return retval;
}

int64_t
ProxyDataset::getMinTimestamp() const {
    int64_t retval;
    MPI_RANK_0_ONLY { retval = impl->getMinTimestamp(); }
    MPI_BROADCAST_RESULT(retval);
    return retval;
}

int64_t
ProxyDataset::getMaxTimestamp() const {
    int64_t retval;
    MPI_RANK_0_ONLY { retval = impl->getMaxTimestamp(); }
    MPI_BROADCAST_RESULT(retval);
    return retval;
}

ProxyDataset::reset() {
    MPI_RANK_0_ONLY { impl->reset(); }
    MPI_BARRIER();
}
