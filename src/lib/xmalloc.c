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
/* -*- mode: C; fill-column: 70; -*- */
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "xmalloc.h"
#include "stinger.h"

#ifdef _OPENMP
#define OMP(x) _Pragma(x)
#else
#define OMP(x)
#endif

/**
* @brief Self-checking wrapper to malloc()
*
* @param sz Number of bytes to allocate
*
* @return 
*/
void *
xmalloc(size_t sz)
{
  void * out;
  out = malloc(sz);
  if (!out) {
    perror("Failed xmalloc: ");
    abort();
  }

  return out;
}

/**
* @brief Self-checking wrapper to calloc()
*
* @param nelem Number of elements
* @param sz Size of each element
*
* @return 
*/
void *
xcalloc(size_t nelem, size_t sz)
{
  void * out;
  out = calloc(nelem, sz);
  if (!out) {
    perror("Failed xcalloc: ");
    abort();
  }
  return out;
}

/**
* @brief Self-checking wrapper to realloc()
*
* @param p Pointer to current allocation
* @param sz Number of bytes of new allocation
*
* @return 
*/
void *
xrealloc(void *p, size_t sz)
{
  void * out;
  out = realloc(p, sz);
  if (!out) {
    perror("Failed xrealloc: ");
    abort();
  }
  return out;
}

/**
* @brief Self-checking wrapper to mmap()
*
* @param addr Starting address for new mapping
* @param len Length of the mapping
* @param prot Memory protection of the mapping
* @param flags MAP_SHARED or MAP_PRIVATE, plus other flags
* @param fd File descriptor
* @param offset Offset
*
* @return 
*/
void *
xmmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
  void * out;
  out = mmap(addr, len, prot, flags, fd, offset);
  if (MAP_FAILED != out)
    return out;
  perror("mmap failed");
  abort();
  return NULL;
}

/**
* @brief Initialize all elements of an array
*
* @param s Array to initialize
* @param c Value to set
* @param num Number of elements in the array
*/
void xelemset(int64_t *s, int64_t c, int64_t num)
{
  assert(s != NULL);
  OMP("omp parallel for")
    for (int64_t v = 0; v < num; ++v) {
      s[v] = c;
    }
}

/**
* @brief Copy from one array to another
*
* @param dest Output array
* @param src Input array
* @param num Number of elements to copy
*/
void xelemcpy(int64_t *dest, int64_t *src, int64_t num)
{
  assert(dest != NULL && src != NULL);
#ifdef _OPENMP
  OMP("omp parallel for")
    for (int64_t v = 0; v < num; ++v)
    {
      dest[v] = src[v];
    }
#else
  memcpy(dest, src, num * sizeof(int64_t));
#endif
}


/**
 * @brief Write zeros to sz bytes starting at x.
 *
 * @param x Pointer to memory location
 * @param sz Memory size in bytes
 */
void xzero(void *x, const size_t sz)
{
  memset (x, 0, sz);
}
