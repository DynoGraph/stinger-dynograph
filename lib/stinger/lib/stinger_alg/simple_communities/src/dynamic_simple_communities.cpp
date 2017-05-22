#include <stdint.h>

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"
#include "dynamic_simple_communities.h"

using namespace gt::stinger;

#define UPDATE_EXTERNAL_CMAP(CS) do {                   \
    OMP("omp parallel for")                             \
      for (intvtx_t k = 0; k < CS.graph_nv; ++k)        \
        cmap[k] = CS.cmap[k];                           \
  } while (0)

std::string
SimpleCommunities::getName() { return "simple_communities"; }

int64_t
SimpleCommunities::getDataPerVertex() { return sizeof(int64_t); }

std::string
SimpleCommunities::getDataDescription() { return "l community_label"; }

void
SimpleCommunities::onInit(stinger_registered_alg * alg)
{
  iter = 0;
  cmap = (int64_t *)alg->alg_data;

  const struct stinger * S = alg->stinger;
  init_cstate_from_stinger (&cstate, S);
  UPDATE_EXTERNAL_CMAP(cstate);
  finalize_community_state (&cstate);
}

void
SimpleCommunities::onPre(stinger_registered_alg * alg)
{
    /* nothing to do */
}

void
SimpleCommunities::onPost(stinger_registered_alg * alg)
{
  ++iter;
  const struct stinger * S = alg->stinger;
  struct community_state cstate_iter;
  intvtx_t ncomm;
  init_cstate_from_stinger (&cstate_iter, S);
  UPDATE_EXTERNAL_CMAP(cstate_iter);
  ncomm = cstate_iter.cg.nv;
  finalize_community_state (&cstate_iter);
  LOG_V_A("simple_communities: ncomm %ld = %ld", (long)iter, (long)ncomm);
}
