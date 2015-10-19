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
#include "stinger_atomics.h"
#include "stinger_bench.h"
#include "xmalloc.h"

// TODO Move this into stinger_alg
int64_t
parallel_breadth_first_search (struct stinger * S, int64_t nv,
				      int64_t source, int64_t * marks,
                                      int64_t * queue, int64_t * Qhead, int64_t * level)
{
  for (int64_t i = 0; i < nv; i++) {
    level[i] = -1;
    marks[i] = 0;
  }

  int64_t nQ, Qnext, Qstart, Qend;
  /* initialize */
  queue[0] = source;
  level[source] = 0;
  marks[source] = 1;
  Qnext = 1;  /* next open slot in the queue */
  nQ = 1;     /* level we are currently processing */
  Qhead[0] = 0;  /* beginning of the current frontier */
  Qhead[1] = 1;  /* end of the current frontier */

  Qstart = Qhead[nQ-1];
  Qend = Qhead[nQ];

  while (Qstart != Qend) {
    OMP ("omp parallel for")
    for (int64_t j = Qstart; j < Qend; j++) {
      STINGER_FORALL_EDGES_OF_VTX_BEGIN (S, queue[j]) {
	int64_t d = level[STINGER_EDGE_DEST];
	if (d < 0) {
	  if (stinger_int64_fetch_add (&marks[STINGER_EDGE_DEST], 1) == 0) {
	    level[STINGER_EDGE_DEST] = nQ;

	    int64_t mine = stinger_int64_fetch_add(&Qnext, 1);
	    queue[mine] = STINGER_EDGE_DEST;
	  }
	}
      } STINGER_FORALL_EDGES_OF_VTX_END();
    }

    Qstart = Qhead[nQ-1];
    Qend = Qnext;
    Qhead[nQ++] = Qend;
  }

  return nQ;
}
int
main (int argc, char ** argv)
{
  stinger_t * S = stinger_new();

  /* insert your data now */
  load_benchmark_data_into_stinger (S, argv[1], 0);

  int64_t source_vertex = 3;

  if (argc > 2) {
      source_vertex = atoll(argv[2]);
  }

  /* get number of vertices */
  uint64_t nv = stinger_max_active_vertex (S);
  nv++;

  print_fragmentation_stats (S, nv);

  /* auxiliary data structure */
  int64_t * marks = xmalloc (nv * sizeof(int64_t));
  int64_t * queue = xmalloc (nv * sizeof(int64_t));
  int64_t * Qhead = xmalloc (nv * sizeof(int64_t));
  int64_t * level = xmalloc (nv * sizeof(int64_t));

  /* the algorithm itself (timed) */
  bench_start();
  int64_t levels = parallel_breadth_first_search (S, nv, source_vertex, marks, queue, Qhead, level);
  bench_end();
  printf("Done.\n");

  if (levels < 5)
    printf("WARNING: Breadth-first search was only %ld levels.  Consider choosing a different source vertex.\n", levels);

  free (level);
  free (Qhead);
  free (queue);
  free (marks);
  stinger_free_all (S);
  return 0;
}
