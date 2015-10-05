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
convert_to_raw_graph (stinger_t * S, char * in_filename, char * out_filename)
{
    printf("Converting %s to %s...\n", in_filename, out_filename);
    load_benchmark_data_into_stinger(S, in_filename, 0);
    FILE* out = fopen(out_filename, "wb");
    fwrite(sizeof(stinger_t), 1, sizeof(size_t), out);
    if (!fwrite(S, sizeof(stinger_t), 1, out))
    {
        printf("Failed to save raw graph!\n");
        return -1;
    }
    fclose(out);
    printf("Done\n");
}

int
main (int argc, char ** argv)
{
    stinger_t * S = stinger_new();
    if (argc < 3)
    {
        printf("Usage: %s input.graph.el output.graph.bin\n", argv[0]);
        exit(1);
    }

    convert_to_raw_graph(S, argv[1], argv[2]); 
    stinger_free_all (S);
    return 0;
}
