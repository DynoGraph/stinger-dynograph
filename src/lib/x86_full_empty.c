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

#include  "x86_full_empty.h"

#if !defined(__MTA__) /* x86 only */

uint64_t 
readfe(uint64_t * v) {
  uint64_t val;
  while(1) {
    val = *v;
    while(val == MARKER) {
      val = *v;
    }
    if(val == stinger_int64_cas(v, val, MARKER))
      break;
  }
  return val;
}

uint64_t
writeef(uint64_t * v, uint64_t new_val) {
  uint64_t val;
  while(1) {
    val = *v;
    while(val != MARKER) {
      val = *v;
    }
    if(MARKER == stinger_int64_cas(v, MARKER, new_val))
      break;
  }
  return val;
}

uint64_t
readff(uint64_t * v) {
  uint64_t val = *v;
  while(val == MARKER) {
    val = *v;
  }
  return val;
}

uint64_t
writeff(uint64_t * v, uint64_t new_val) {
  uint64_t val;
  while(1) {
    val = *v;
    while(val == MARKER) {
      val = *v;
    }
    if(val == stinger_int64_cas(v, val, new_val))
      break;
  }
  return val;
}

uint64_t
writexf(uint64_t * v, uint64_t new_val) {
  *v = new_val;
  return new_val;
}

#endif  /* x86 only */

