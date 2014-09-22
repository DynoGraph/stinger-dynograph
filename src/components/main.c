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

/*
 * Perform a shiloach vishkin connected components calculation in parallel on a stinger graph
 */
int64_t
parallel_shiloach_vishkin_components (struct stinger * S, int64_t nv,
                                      int64_t * component_map)
{
  /* Initialize each vertex with its own component label in parallel */
  OMP ("omp parallel for")
  for (uint64_t i = 0; i < nv; i++) {
    component_map[i] = i;
  }

  /* Iterate until no changes occur */
  while (1) {
    int changed = 0;

    /* For all edges in the STINGER graph of type 0 in parallel, attempt to assign
       lesser component IDs to neighbors with greater component IDs */
    for (int64_t t = 0; t < STINGER_NUMETYPES; t++) {
      STINGER_PARALLEL_FORALL_EDGES_BEGIN (S, t) {
	if (component_map[STINGER_EDGE_DEST] <
	    component_map[STINGER_EDGE_SOURCE]) {
	  component_map[STINGER_EDGE_SOURCE] = component_map[STINGER_EDGE_DEST];
	  changed++;
	}
      }
      STINGER_PARALLEL_FORALL_EDGES_END ();
    }

    /* if nothing changed */
    if (!changed)
      break;

    /* Tree climbing with OpenMP parallel for */
    OMP ("omp parallel for")
    for (uint64_t i = 0; i < nv; i++) {
      while (component_map[i] != component_map[component_map[i]])
	component_map[i] = component_map[component_map[i]];
    }
  }

  return 0;
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
  int64_t * component_labels = xmalloc (nv * sizeof(int64_t));


  /* the algorithm itself (timed) */
  bench_start();
  parallel_shiloach_vishkin_components (S, nv, component_labels);
  bench_end();
  printf("Done.\n");


  free (component_labels);
  stinger_free_all (S);
  return 0;
}
