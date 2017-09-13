#include <stdint.h>

#include "stinger_core/stinger.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"
#include "stinger_utils/timer.h"
extern "C" {
#include "stinger_alg/clustering.h"
}
#include "dynamic_clustering.h"

using namespace gt::stinger;

std::string
ClusteringCoefficients::getName() { return "clustering_coeff"; }

int64_t
ClusteringCoefficients::getDataPerVertex() { return sizeof(double) + sizeof(int64_t); }

std::string
ClusteringCoefficients::getDataDescription() { return "dl coeff ntriangles"; }

ClusteringCoefficients::ClusteringCoefficients() { init_timer(); }

void
ClusteringCoefficients::onInit(stinger_registered_alg * alg)
{
    local_cc = (double *)alg->alg_data;
    ntri = (int64_t *)(((double *)alg->alg_data) + alg->stinger->max_nv);
    affected = (int64_t*) xcalloc (alg->stinger->max_nv, sizeof (int64_t *));

    LOG_I("Clustering coefficients init starting");
    tic();
    count_all_triangles (alg->stinger, ntri);

    OMP("omp parallel for")
    for(uint64_t v = 0; v < alg->stinger->max_nv; v++) {
      int64_t deg = stinger_outdegree_get(alg->stinger, v);
      int64_t d = deg * (deg-1);
      local_cc[v] = (d ? ntri[v] / (double) d : 0.0);
    }
    double time = toc();
    LOG_I("Clustering coefficients init end");
    LOG_I_A("Time: %f sec", time);
}

void
ClusteringCoefficients::onPre(stinger_registered_alg * alg)
{
    tic();

    OMP("omp parallel for")
    for (uint64_t v = 0; v < alg->stinger->max_nv; v++) {
      affected[v] = 0;
    }

    /* each vertex incident on an insertion is affected */
    OMP("omp parallel for")
    for (uint64_t i = 0; i < alg->num_insertions; i++) {
      int64_t src = alg->insertions[i].source;
      int64_t dst = alg->insertions[i].destination;
      stinger_int64_fetch_add (&affected[src], 1);
      stinger_int64_fetch_add (&affected[dst], 1);

      /* and all neighbors */
      STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(alg->stinger, src) {
        stinger_int64_fetch_add (&affected[STINGER_EDGE_DEST], 1);
      } STINGER_FORALL_OUT_EDGES_OF_VTX_END();

      STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(alg->stinger, dst) {
        stinger_int64_fetch_add (&affected[STINGER_EDGE_DEST], 1);
      } STINGER_FORALL_OUT_EDGES_OF_VTX_END();
    }

    /* each vertex incident on a deletion is affected */
    OMP("omp parallel for")
    for (uint64_t i = 0; i < alg->num_deletions; i++) {
      int64_t src = alg->deletions[i].source;
      int64_t dst = alg->deletions[i].destination;
      stinger_int64_fetch_add (&affected[src], 1);
      stinger_int64_fetch_add (&affected[dst], 1);

      /* and all neighbors */
      STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(alg->stinger, src) {
        stinger_int64_fetch_add (&affected[STINGER_EDGE_DEST], 1);
      } STINGER_FORALL_OUT_EDGES_OF_VTX_END();

      STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(alg->stinger, dst) {
        stinger_int64_fetch_add (&affected[STINGER_EDGE_DEST], 1);
      } STINGER_FORALL_OUT_EDGES_OF_VTX_END();
    }

    double time = toc();
    LOG_I_A("Time: %f", time);
}

void
ClusteringCoefficients::onPost(stinger_registered_alg * alg)
{
    tic();

    OMP("omp parallel for")
    for (uint64_t v = 0; v < alg->stinger->max_nv; v++) {
      if (affected[v]) {
        ntri[v] = count_triangles (alg->stinger, v);

        int64_t deg = stinger_outdegree_get(alg->stinger, v);
        int64_t d = deg * (deg-1);
        local_cc[v] = (d ? ntri[v] / (double) d : 0.0);
      }
    }

    double time = toc();
    LOG_I_A("Time: %f", time);
}

ClusteringCoefficients::~ClusteringCoefficients()
{
    xfree(affected);
}
