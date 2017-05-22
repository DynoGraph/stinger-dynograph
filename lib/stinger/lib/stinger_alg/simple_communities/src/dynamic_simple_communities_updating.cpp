#include <stdint.h>
#include <stinger_net/stinger_alg.h>

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"
#include "dynamic_simple_communities_updating.h"

using namespace gt::stinger;

#define UPDATE_EXTERNAL_CMAP(CS) do {                   \
    OMP("omp parallel for")                             \
      for (intvtx_t k = 0; k < CS.graph_nv; ++k)        \
        cmap[k] = CS.cmap[k];                           \
  } while (0)

std::string
SimpleCommunitiesUpdating::getName() { return "simple_communities_updating"; }

int64_t
SimpleCommunitiesUpdating::getDataPerVertex() { return sizeof(int64_t); }

std::string
SimpleCommunitiesUpdating::getDataDescription() { return "l community_label"; }

SimpleCommunitiesUpdating::SimpleCommunitiesUpdating(int initial_compute)
{
  this->initial_compute = initial_compute;
}

void
SimpleCommunitiesUpdating::onInit(stinger_registered_alg * alg)
{
  iter = 0;
  cmap = (int64_t *)alg->alg_data;
  //nv = (stinger_max_active_vertex(alg->stinger))?stinger_max_active_vertex(alg->stinger) + 1:0;
  nv = alg->stinger->max_nv;

  const struct stinger * S = alg->stinger;
  if (initial_compute) {
    init_cstate_from_stinger (&cstate, S);
  } else {
    init_empty_community_state (&cstate, nv, (1+stinger_total_edges(S))/2);
  }
  UPDATE_EXTERNAL_CMAP(cstate);
}

void
SimpleCommunitiesUpdating::onPre(stinger_registered_alg * alg)
{
  /* Must be here to find the removal weights. */
  cstate_preproc_alg (&cstate, alg);
}

void
SimpleCommunitiesUpdating::onPost(stinger_registered_alg * alg)
{
  ++iter;
  cstate_update (&cstate, alg->stinger);
  UPDATE_EXTERNAL_CMAP(cstate);
  LOG_V_A("simple_communities: ncomm %ld = %ld", (long)iter, (long)cstate.cg.nv);
}
