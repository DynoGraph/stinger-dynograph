/*
 * GraphBench is a benchmark suite for
 *      microarchitectural simulation of streaming graph workloads
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
#include "hooks.h"
#include <stdio.h>

#if defined(ENABLE_SNIPER_HOOKS)
    #include <hooks_base.h>
#elif defined(ENABLE_PIN_HOOKS)
#endif

int __attribute__ ((noinline)) bench_start() {
    printf("[DynoGraph] Entering ROI...\n");

    #if defined(ENABLE_SNIPER_HOOKS)
        parmacs_roi_begin();
    #elif defined(ENABLE_PIN_HOOKS)
        __asm__("");
    #endif

    return 0;
}

int __attribute__ ((noinline)) bench_end() {
    #if defined(ENABLE_SNIPER_HOOKS)
        parmacs_roi_end();
    #elif defined(ENABLE_PIN_HOOKS)
        __asm__("");
    #endif

    printf("[DynoGraph] Exiting ROI...\n");
    return 0;
}

