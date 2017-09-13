#include <stdint.h>

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"
extern "C" {
#include "stinger_alg/static_components.h"
}
#include "dynamic_static_components.h"

using namespace gt::stinger;

std::string
ConnectedComponents::getName() { return "static_components"; }

int64_t
ConnectedComponents::getDataPerVertex() { return sizeof(int64_t); }

std::string
ConnectedComponents::getDataDescription()
{
    return "l component_label";
}

void
ConnectedComponents::onInit(stinger_registered_alg * alg)
{
    components = (int64_t *)alg->alg_data;

    parallel_shiloach_vishkin_components(alg->stinger, alg->max_active_vertex+1, components);
}

void
ConnectedComponents::onPre(stinger_registered_alg * alg)
{
    /* nothing to do */
}

void
ConnectedComponents::onPost(stinger_registered_alg * alg)
{
    parallel_shiloach_vishkin_components(alg->stinger, alg->max_active_vertex+1, components);
}
