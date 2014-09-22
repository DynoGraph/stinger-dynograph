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
/* x86 Emulation of Full Empty Bits Using atomic check-and-swap.
 *
 * NOTES:
 * - Using these functions means that the MARKER value defined 
 *   below must be reserved in your application and CANNOT be 
 *   considered a normal value.  Feel free to change the value to
 *   suit your application.
 * - Do not compile this file with optimization.  There are loops
 *   that do not make sense in a serial context.  The compiler will
 *   optimize them out.
 * - Improper use of these functions can and will result in deadlock.
 *
 * Original author: rmccoll3@gatech.edu
 */

#ifndef	X86_FULL_EMPTY_C
#define	X86_FULL_EMPTY_C

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

#include    "stinger_atomics.h"

#if !defined(__MTA__) /* x86 only */

#include  <stdint.h>
#define MARKER UINT64_MAX

uint64_t 
readfe(uint64_t * v);

uint64_t
writeef(uint64_t * v, uint64_t new_val);

uint64_t
readff(uint64_t * v);

uint64_t
writeff(uint64_t * v, uint64_t new_val);

uint64_t
writexf(uint64_t * v, uint64_t new_val);

#endif  /* x86 only */

#ifdef __cplusplus
}
#undef restrict
#endif

#endif  /*X86-FULL-EMPTY_C*/

