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

int
main (int argc, char ** argv)
{
  stinger_t * S = stinger_new();

  /* insert your data now */
  load_benchmark_data_into_stinger (S, argv[1],1);

  int64_t source_vertex = 3;

  if (argc > 2) {
    source_vertex = atoll(argv[2]);
  }

  /* get number of vertices */
  uint64_t nv = stinger_max_active_vertex (S);
  nv++;

  print_fragmentation_stats (S, nv);

  stinger_free_all (S);
  return 0;
}
