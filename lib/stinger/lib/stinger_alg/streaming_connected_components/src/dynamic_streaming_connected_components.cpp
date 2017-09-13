#include <stdint.h>

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"
extern "C" {
#include "stinger_alg/streaming_connected_components.h"
}
#include "dynamic_streaming_connected_components.h"

using namespace gt::stinger;

std::string
StreamingConnectedComponents::getName() { return "streaming_connected_components"; }

int64_t
StreamingConnectedComponents::getDataPerVertex() { return 2*sizeof(int64_t); }

std::string
StreamingConnectedComponents::getDataDescription()
{
    return "ll component_label component_size";
}

void
StreamingConnectedComponents::onInit(stinger_registered_alg * alg)
{
    bzero(alg->alg_data, sizeof(int64_t) * 2 * alg->stinger->max_nv);
    components = (int64_t *)alg->alg_data;
    component_size  = components + alg->stinger->max_nv;

    stinger_scc_initialize_internals(alg->stinger,alg->stinger->max_nv,&scc_internal,4);
    stinger_scc_reset_stats(&stats);
}

void
StreamingConnectedComponents::onPre(stinger_registered_alg * alg)
{
    /* nothing to do */
}

void
StreamingConnectedComponents::onPost(stinger_registered_alg * alg)
{
    stinger_scc_insertion(alg->stinger,alg->stinger->max_nv,scc_internal,&stats,alg->insertions,alg->num_insertions);
    stinger_scc_deletion(alg->stinger,alg->stinger->max_nv,scc_internal,&stats,alg->deletions,alg->num_deletions);

    stinger_scc_copy_component_array(scc_internal, (int64_t *)alg->alg_data);
}


StreamingConnectedComponents::~StreamingConnectedComponents()
{
    stinger_scc_release_internals(&scc_internal);
}