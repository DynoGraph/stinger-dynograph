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
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>

#if !defined(XMALLOC_H_)
#define XMALLOC_H_

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

#if defined(__GNUC__)
#define FNATTR_MALLOC __attribute__((malloc))
#else
#define FNATTR_MALLOC
#endif

void * xmalloc (size_t) FNATTR_MALLOC;
void * xcalloc (size_t, size_t) FNATTR_MALLOC;
void * xrealloc (void *, size_t) FNATTR_MALLOC;
void * xmmap (void *addr, size_t len, int prot, int flags, int fd, off_t offset);
void xelemset (int64_t *s, int64_t c, int64_t n);
void xelemcpy (int64_t *dest, int64_t *src, int64_t n);
void xzero (void *x, const size_t sz);

/**
* @brief Self-checking wrapper to free()
*
* @param p Pointer to allocation
*/
#define xfree(p) \
{ \
  if (p != NULL) { \
    free(p); \
    p = NULL; \
  } \
}

#ifdef __cplusplus
}
#undef restrict
#endif

#endif /* XMALLOC_H_ */
