/* 
 * GraphBench is a benchmark suite for 
 *		microarchitectural simulation of streaming graph workloads
 * 
 * Copyright (C) 2014  Georgia Tech Research Institute
 * Jason Poovey (jason.poovey@gtri.gatech.edu)
 * David Ediger (david.ediger@gtri.gatech.edu)
 * Eric Hein (eric.hein@gtri.gatech.edu)
 * Tom Conte (tom@conte.us)

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdint.h>

#include "hooks.h"
#include "stinger.h"
#include "stinger_bench.h"
#include "xmalloc.h"

int64_t
pagerank (stinger_t * S, int64_t NV, double * pr, double * tmp_pr_in, double epsilon, double dampingfactor, int64_t maxiter)
{
  double * tmp_pr = NULL;
  if(tmp_pr_in) {
    tmp_pr = tmp_pr_in;
  } else {
    tmp_pr = (double *) xmalloc (sizeof(double) * NV);
  }

  int64_t iter = maxiter;
  double delta = 1;

  while (delta > epsilon && iter > 0) {
    OMP("omp parallel for")
    for (uint64_t v = 0; v < NV; v++) {
      tmp_pr[v] = 0;

      STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, v) {
	tmp_pr[v] += (((double)pr[STINGER_EDGE_DEST]) / 
	  ((double) stinger_outdegree_get(S, STINGER_EDGE_DEST)));
      } STINGER_FORALL_EDGES_OF_VTX_END();
    }

    OMP("omp parallel for")
    for (uint64_t v = 0; v < NV; v++) {
      tmp_pr[v] = tmp_pr[v] * dampingfactor + (((double)(1-dampingfactor)) / ((double)NV));
    }

    delta = 0;
    OMP("omp parallel for reduction(+:delta)")
    for (uint64_t v = 0; v < NV; v++) {
      double mydelta = tmp_pr[v] - pr[v];
      if(mydelta < 0)
	mydelta = -mydelta;
      delta += mydelta;
    }

    OMP("omp parallel for")
    for (uint64_t v = 0; v < NV; v++) {
      pr[v] = tmp_pr[v];
    }
  }

  if (!tmp_pr_in)
    free(tmp_pr);
}

int
main (int argc, char ** argv)
{
  stinger_t * S = stinger_new();

  /* insert your data now */
  load_benchmark_data_into_stinger (S, argv[1],0);


  /* get number of vertices */
  uint64_t nv = stinger_max_active_vertex (S);
  nv++;

  print_fragmentation_stats (S, nv);


  /* auxiliary data structure */
  double * pagerank_scores = xmalloc (nv * sizeof(int64_t));
  double * pagerank_scores_tmp = xmalloc (nv * sizeof(int64_t));


  /* the algorithm itself (timed) */
  bench_start();
  pagerank (S, nv, pagerank_scores, pagerank_scores_tmp, 1e-8, 0.85, 100);
  bench_end();
  printf("Done.\n");


  /* cleanup */
  free (pagerank_scores);
  free (pagerank_scores_tmp);
  stinger_free_all (S);
  return 0;
}
