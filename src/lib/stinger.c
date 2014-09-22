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
/* -*- mode: C; mode: folding; fill-column: 70; -*- */

#if defined(_OPENMP)
#include <omp.h>
#endif

#include "stinger.h"
#include "stinger_atomics.h"
#include "xmalloc.h"
#include "x86_full_empty.h"


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * ACCESS INTERNAL "CLASSES"
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
inline stinger_vertices_t *
stinger_vertices_get(const stinger_t * S) {
  MAP_STING(S);
  return vertices;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * EXTERNAL INTERFACE FOR VERTICES
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* IN DEGREE */

inline vdegree_t
stinger_indegree_get(const stinger_t * S, vindex_t v) {
  return stinger_vertex_indegree_get(stinger_vertices_get(S), v);
}

inline vdegree_t
stinger_indegree_set(const stinger_t * S, vindex_t v, vdegree_t d) {
  return stinger_vertex_indegree_set(stinger_vertices_get(S), v, d);
}

inline vdegree_t
stinger_indegree_increment(const stinger_t * S, vindex_t v, vdegree_t d) {
  return stinger_vertex_indegree_increment(stinger_vertices_get(S), v, d);
}

inline vdegree_t
stinger_indegree_increment_atomic(const stinger_t * S, vindex_t v, vdegree_t d) {
  return stinger_vertex_indegree_increment_atomic(stinger_vertices_get(S), v, d);
}

/* OUT DEGREE */

inline vdegree_t
stinger_outdegree_get(const stinger_t * S, vindex_t v) {
  return stinger_vertex_outdegree_get(stinger_vertices_get(S), v);
}

inline vdegree_t
stinger_outdegree_set(const stinger_t * S, vindex_t v, vdegree_t d) {
  return stinger_vertex_outdegree_set(stinger_vertices_get(S), v, d);
}

inline vdegree_t
stinger_outdegree_increment(const stinger_t * S, vindex_t v, vdegree_t d) {
  return stinger_vertex_outdegree_increment(stinger_vertices_get(S), v, d);
}

inline vdegree_t
stinger_outdegree_increment_atomic(const stinger_t * S, vindex_t v, vdegree_t d) {
  return stinger_vertex_outdegree_increment_atomic(stinger_vertices_get(S), v, d);
}

/* WEIGHT */

vweight_t
stinger_vweight_get(const stinger_t * S, vindex_t v) {
  MAP_STING(S);
  return stinger_vertex_weight_get(vertices, v);
}

vweight_t
stinger_vweight_set(const stinger_t * S, vindex_t v, vweight_t weight) {
  MAP_STING(S);
  return stinger_vertex_weight_set(vertices, v, weight);
}

vweight_t
stinger_vweight_increment(const stinger_t * S, vindex_t v, vweight_t weight) {
  MAP_STING(S);
  return stinger_vertex_weight_increment(vertices, v, weight);
}

vweight_t
stinger_vweight_increment_atomic(const stinger_t * S, vindex_t v, vweight_t weight) {
  MAP_STING(S);
  return stinger_vertex_weight_increment_atomic(vertices, v, weight);
}

/* ADJACENCY */
adjacency_t
stinger_adjacency_get(const stinger_t * S, vindex_t v) {
  MAP_STING(S);
  return stinger_vertex_edges_get(vertices, v);
}


/* {{{ Edge block pool */
/* TODO XXX Rework / possibly move EB POOL functions */



static void
get_from_ebpool (const struct stinger * S, eb_index_t *out, size_t k)
{
  MAP_STING(S);
  eb_index_t ebt0;
  {
    ebt0 = stinger_int64_fetch_add (&(ebpool->ebpool_tail), k);
    if (ebt0 + k >= EBPOOL_SIZE) {
      fprintf (stderr, "XXX: eb pool exhausted\n");
      abort ();
    }
    OMP("omp parallel for")
      for (size_t ki = 0; ki < k; ++ki)
        out[ki] = ebt0 + ki;
  }
}

/* }}} */

/* {{{ Internal utilities */


/** @brief Calculate the largest active vertex ID
 *
 *  Finds the largest vertex ID whose in-degree and/or out-degree
 *  is greater than zero.
 *
 *  <em>NOTE:</em> If you are using this to obtain a
 *  value of "nv" for additional STINGER calls, you must add one to the 
 *  result.
 *
 *  @param S The STINGER data structure
 *  @return Largest active vertex ID
 */
uint64_t
stinger_max_active_vertex(const struct stinger * S) {
  uint64_t out = 0;
  OMP("omp parallel") {
    uint64_t local_max = 0;
    OMP("omp for")
    for(uint64_t i = 0; i < STINGER_MAX_LVERTICES; i++) {
      if((stinger_indegree_get(S, i) > 0 || stinger_outdegree_get(S, i) > 0) && 
	i > local_max) {
	local_max = i;
      }
    }
    OMP("omp critical") {
      if(local_max > out)
	out = local_max;
    }
  }
  return out;
}

/** @brief Calculate the number of active vertices
 *
 *  Counts the number of vertices whose in-degree is greater than zero and
 *  out-degree is greater than zero.
 *
 *  @param S The STINGER data structure
 *  @return Number of active vertices
 */
uint64_t
stinger_num_active_vertices(const struct stinger * S) {
  uint64_t out = 0;
  OMP("omp parallel for reduction(+:out)")
  for(uint64_t i = 0; i < STINGER_MAX_LVERTICES; i++) {
    if(stinger_indegree_get(S, i) > 0 || stinger_outdegree_get(S, i) > 0) {
      out++;
    }
  }
  return out;
}


const struct stinger_eb *
stinger_next_eb (const struct stinger *G,
                 const struct stinger_eb *eb_)
{
  MAP_STING(G);
  return ebpool->ebpool + readff((uint64_t *)&eb_->next);
}

int64_t
stinger_eb_type (const struct stinger_eb * eb_)
{
  return eb_->etype;
}

int
stinger_eb_high (const struct stinger_eb *eb_)
{
  return eb_->high;
}

int
stinger_eb_is_blank (const struct stinger_eb *eb_, int k_)
{
  return eb_->edges[k_].neighbor < 0;
}

int64_t
stinger_eb_adjvtx (const struct stinger_eb * eb_, int k_)
{
  return eb_->edges[k_].neighbor;
}

int64_t
stinger_eb_weight (const struct stinger_eb * eb_, int k_)
{
  return eb_->edges[k_].weight;
}

int64_t
stinger_eb_ts (const struct stinger_eb * eb_, int k_)
{
  return eb_->edges[k_].timeRecent;
}

int64_t
stinger_eb_first_ts (const struct stinger_eb * eb_, int k_)
{
  return eb_->edges[k_].timeFirst;
}

/**
* @brief Count the total number of edges in STINGER.
*
* @param S The STINGER data structure
*
* @return The number of edges in STINGER
*/
int64_t
stinger_total_edges (const struct stinger * S)
{
  uint64_t rtn = 0;
  for (uint64_t i = 0; i < STINGER_MAX_LVERTICES; i++) {
    rtn += stinger_outdegree_get(S, i);
  }
  return rtn;
}

/**
* @brief Count the number of edges in STINGER up to nv.
*
* @param S The STINGER data structure
* @param nv The maximum vertex ID to count.
*
* @return The number of edges in STINGER held by vertices 0 through nv-1
*/
int64_t
stinger_edges_up_to(const struct stinger * S, int64_t nv)
{
  uint64_t rtn = 0;
  for (uint64_t i = 0; i < nv; i++) {
    rtn += stinger_outdegree_get(S, i);
  }
  return rtn;
}

/**
* @brief Calculate the total size of the active STINGER graph in memory.
*
* @param S The STINGER data structure
*
* @return The number of bytes currently in use by STINGER
*/
size_t
stinger_graph_size (const struct stinger *S)
{
  MAP_STING(S);
  int64_t num_edgeblocks = ETA->high;
  int64_t size_edgeblock = sizeof(struct stinger_eb);

  int64_t vertices_size = stinger_vertices_size_bytes(stinger_vertices_get(S));

  int64_t result = (num_edgeblocks * size_edgeblock) + vertices_size;

  return result;
}

void
stinger_print_eb(struct stinger_eb * eb) {
  printf(
    "EB VTX:  %ld\n"
    "  NEXT:    %ld\n"
    "  TYPE:    %ld\n"
    "  NUMEDGS: %ld\n"
    "  HIGH:    %ld\n"
    "  SMTS:    %ld\n"
    "  LGTS:    %ld\n"
    "  EDGES:\n",
    eb->vertexID, eb->next, eb->etype, eb->numEdges, eb->high, eb->smallStamp, eb->largeStamp);
  uint64_t j = 0;
  for (; j < eb->high && j < STINGER_EDGEBLOCKSIZE; j++) {
    printf("    TO: %s%ld WGT: %ld TSF: %ld TSR: %ld\n", 
      eb->edges[j].neighbor < 0 ? "x " : "  ", eb->edges[j].neighbor < 0 ? ~(eb->edges[j].neighbor) : eb->edges[j].neighbor, 
      eb->edges[j].weight, eb->edges[j].timeFirst, eb->edges[j].timeRecent);
  }
  if(j < STINGER_EDGEBLOCKSIZE) {
    printf("  ABOVE HIGH:\n");
    for (; j < STINGER_EDGEBLOCKSIZE; j++) {
      printf("    TO: %ld WGT: %ld TSF: %ld TSR: %ld\n", 
	eb->edges[j].neighbor, eb->edges[j].weight, eb->edges[j].timeFirst, eb->edges[j].timeRecent);
    }
  }
}

/**
* @brief Checks the STINGER metadata for inconsistencies.
*
* @param S The STINGER data structure
* @param NV The total number of vertices
*
* @return 0 on success; failure otherwise
*/
uint32_t
stinger_consistency_check (struct stinger *S, uint64_t NV)
{
  uint32_t returnCode = 0;
  uint64_t *inDegree = xcalloc (NV, sizeof (uint64_t));
  if (inDegree == NULL) {
    returnCode |= 0x00000001;
    return returnCode;
  }

  MAP_STING(S);
  struct stinger_eb * ebpool_priv = ebpool->ebpool;
  // check blocks
  OMP("omp parallel for reduction(|:returnCode)")
  for (uint64_t i = 0; i < NV; i++) {
    uint64_t curOutDegree = 0;
    const struct stinger_eb *curBlock = ebpool_priv + stinger_vertex_edges_get(vertices, i);
    while (curBlock != ebpool_priv) {
      if (curBlock->vertexID != i)
        returnCode |= 0x00000002;
      if (curBlock->high > STINGER_EDGEBLOCKSIZE)
        returnCode |= 0x00000004;

      int64_t numEdges = 0;
      int64_t smallStamp = INT64_MAX;
      int64_t largeStamp = INT64_MIN;

      uint64_t j = 0;
      for (; j < curBlock->high && j < STINGER_EDGEBLOCKSIZE; j++) {
        if (!stinger_eb_is_blank (curBlock, j)) {
          stinger_int64_fetch_add (&inDegree[stinger_eb_adjvtx (curBlock, j)], 1);
          curOutDegree++;
          numEdges++;
          if (stinger_eb_ts (curBlock, j) < smallStamp)
            smallStamp = stinger_eb_ts (curBlock, j);
          if (stinger_eb_first_ts (curBlock, j) < smallStamp)
            smallStamp = stinger_eb_first_ts (curBlock, j);
          if (stinger_eb_ts (curBlock, j) > largeStamp)
            largeStamp = stinger_eb_ts (curBlock, j);
          if (stinger_eb_first_ts (curBlock, j) > largeStamp)
            largeStamp = stinger_eb_first_ts (curBlock, j);
        }
      }
      if (numEdges && numEdges != curBlock->numEdges)
        returnCode |= 0x00000008;
      if (numEdges && largeStamp > curBlock->largeStamp)
        returnCode |= 0x00000010;
      if (numEdges && smallStamp < curBlock->smallStamp)
        returnCode |= 0x00000020;
      for (; j < STINGER_EDGEBLOCKSIZE; j++) {
        if (!(stinger_eb_is_blank (curBlock, j) ||
              (stinger_eb_adjvtx (curBlock, j) == 0
               && stinger_eb_weight (curBlock, j) == 0
               && stinger_eb_ts (curBlock, j) == 0
               && stinger_eb_first_ts (curBlock, j) == 0)))
          returnCode |= 0x00000040;
      }
      curBlock = ebpool_priv + curBlock->next;
    }

    if (curOutDegree != stinger_outdegree_get(S, i))
      returnCode |= 0x00000080;
  }

  OMP("omp parallel for reduction(|:returnCode)")
  for (uint64_t i = 0; i < NV; i++) {
    if (inDegree[i] != stinger_indegree_get(S,i))
      returnCode |= 0x00000100;
  }

  free (inDegree);

#if STINGER_NUMETYPES == 1
  // check for self-edges and duplicate edges
  int64_t count_self = 0;
  int64_t count_duplicate = 0;

  int64_t * off = NULL;
  int64_t * ind = NULL;

  stinger_to_sorted_csr (S, NV, &off, &ind, NULL, NULL, NULL, NULL);

  OMP ("omp parallel for reduction(+:count_self, count_duplicate)")
  for (int64_t k = 0; k < NV; k++)
  {
    int64_t myStart = off[k];
    int64_t myEnd = off[k+1];

    for (int64_t j = myStart; j < myEnd; j++)
    {
      if (ind[j] == k)
	count_self++;
      if (j < myEnd-1 && ind[j] == ind[j+1])
	count_duplicate++;
    }
  }

  free (ind);
  free (off);

  if (count_self != 0)
    returnCode |= 0x00000200;

  if (count_duplicate != 0)
    returnCode |= 0x00000400;
#endif

  return returnCode;
}


/* }}} */



/* {{{ Allocating and tearing down */

void stinger_init (void)
{
}


/** @brief Create a new STINGER data structure.
 *
 *  Allocates memory for a STINGER data structure.  If this is the first STINGER
 *  to be allocated, it also initializes the edge block pool.  Edge blocks are
 *  allocated and assigned for each value less than STINGER_NUMETYPES.
 *
 *  @return Pointer to struct stinger
 */
struct stinger *stinger_new (void)
{
  size_t i;
  size_t sz = 0;

  size_t vertices_start = 0;
  sz += stinger_vertices_size(STINGER_MAX_LVERTICES);

  size_t physmap_start = sz;
  //sz += stinger_physmap_size(STINGER_MAX_LVERTICES); 

  size_t ebpool_start = sz;
  sz += sizeof(struct stinger_ebpool);

  size_t etype_names_start = sz;
 // sz += stinger_names_size(STINGER_NUMETYPES);

  size_t vtype_names_start = sz;
  //sz += stinger_names_size(STINGER_NUMVTYPES);

  size_t ETA_start = sz;
  sz += STINGER_NUMETYPES * sizeof(struct stinger_etype_array);

  size_t length = sz;

  struct stinger *G = xmalloc (sizeof(struct stinger) + sz);

  xzero(G, sizeof(struct stinger) + sz);
  G->length = length;
  G->vertices_start = vertices_start;
  G->physmap_start = physmap_start;
  G->etype_names_start = etype_names_start;
  G->vtype_names_start = vtype_names_start;
  G->ETA_start = ETA_start;
  G->ebpool_start = ebpool_start;

  MAP_STING(G);

  int64_t zero = 0;
  stinger_vertices_init(vertices, STINGER_MAX_LVERTICES);
  //stinger_physmap_init(physmap, STINGER_MAX_LVERTICES);
  //stinger_names_init(etype_names, STINGER_NUMETYPES);
  //stinger_names_create_type(etype_names, "None", &zero);
  //stinger_names_init(vtype_names, STINGER_NUMVTYPES);
  //stinger_names_create_type(vtype_names, "None", &zero);

  ebpool->ebpool_tail = 1;
  ebpool->is_shared = 0;

#if STINGER_NUMETYPES == 1
  ETA[0].length = EBPOOL_SIZE;
  ETA[0].high = 0;
#else
  OMP ("omp parallel for")
  for (i = 0; i < STINGER_NUMETYPES; ++i) {
    ETA[i].length = EBPOOL_SIZE;
    ETA[i].high = 0;
  }
#endif

  return G;
}

/** @brief Free memory allocated to a particular STINGER instance.
 *
 *  Frees the ETA pointers for each edge type, the LVA, and the struct stinger
 *  itself.  Does not actually free any edge blocks, as there may be other
 *  active STINGER instances.
 *
 *  @param S The STINGER data structure
 *  @return NULL on success
 */
struct stinger *
stinger_free (struct stinger *S)
{
  size_t i;
  if (!S)
    return S;

  free (S);
  return NULL;
}

/** @brief Free the STINGER data structure and all edge blocks.
 *
 *  Free memory allocated to the specified STINGER data structure.  Also frees
 *  the STINGER edge block pool, effectively ending all STINGER operations globally.
 *  Only call this function if you are done using STINGER entirely.
 *
 *  @param S The STINGER data structure
 *  @return NULL on success
 */
struct stinger *
stinger_free_all (struct stinger *S)
{
  struct stinger *out;
  out = stinger_free (S);
  return out;
}

/* TODO inspect possibly move out with other EB POOL stuff */
static eb_index_t new_eb (struct stinger * S, int64_t etype, int64_t from)
{
  MAP_STING(S);
  size_t k;
  eb_index_t out = 0;
  get_from_ebpool (S, &out, 1);
  struct stinger_eb * block = ebpool->ebpool + out;
  assert (block != ebpool->ebpool);
  xzero (block, sizeof (*block));
  block->etype = etype;
  block->vertexID = from;
  block->smallStamp = INT64_MAX;
  block->largeStamp = INT64_MIN;
  return out;
}

void
new_ebs (struct stinger * S, eb_index_t *out, size_t neb, int64_t etype,
         int64_t from)
{
  if (neb < 1)
    return;
  get_from_ebpool (S, out, neb);

  MAP_STING(S);

  OMP ("omp parallel for")
    for (size_t i = 0; i < neb; ++i) {
      struct stinger_eb * block = ebpool->ebpool + out[i];
      xzero (block, sizeof (*block));
      block->etype = etype;
      block->vertexID = from;
      block->smallStamp = INT64_MAX;
      block->largeStamp = INT64_MIN;
    }
}

void
new_blk_ebs (eb_index_t *out, const struct stinger *restrict G,
             const int64_t nvtx, const size_t * restrict blkoff,
             const int64_t etype)
{
  size_t neb;
  if (nvtx < 1)
    return;
  neb = blkoff[nvtx];
  get_from_ebpool (G,out, neb);

  MAP_STING(G);

  OMP ("omp parallel for")
    for (size_t k = 0; k < neb; ++k) {
      struct stinger_eb * block = ebpool->ebpool + out[k];
      xzero (block, sizeof (*block));
      block->etype = etype;
      block->smallStamp = INT64_MAX;
      block->largeStamp = INT64_MIN;
    }

  OMP ("omp parallel for")
    for (int64_t v = 0; v < nvtx; ++v) {
      const int64_t from = v;
      const size_t blkend = blkoff[v + 1];
        for (size_t k = blkoff[v]; k < blkend; ++k)
          ebpool->ebpool[out[k]].vertexID = from;
      if (blkend)
          for (size_t k = blkoff[v]; k < blkend - 1; ++k)
            ebpool->ebpool[out[k]].next = out[k + 1];
    }
}

/* }}} */

void
push_ebs (struct stinger *G, size_t neb,
          eb_index_t * restrict eb)
{
  int64_t etype, place;
  assert (G);
  assert (eb);

  if (!neb)
    return;

  MAP_STING(G);
  etype = ebpool->ebpool[eb[0]].etype;
  assert (etype >= 0);
  assert (etype < STINGER_NUMETYPES);

  place = stinger_int64_fetch_add (&(ETA[etype].high), neb);

  eb_index_t *blocks;
  blocks = ETA[etype].blocks;

  for (int64_t k = 0; k < neb; ++k)
    blocks[place + k] = eb[k];
}

struct curs
etype_begin (const stinger_t * S, int64_t v, int etype)
{
  MAP_STING(S);
  struct curs out;
  assert (vertices);
  out.eb = stinger_vertex_edges_get(vertices,v);
  out.loc = stinger_vertex_edges_pointer_get(vertices,v);
  while (out.eb && ebpool->ebpool[out.eb].etype != etype) {
    out.loc = &(ebpool->ebpool[out.eb].next);
    out.eb = readff((uint64_t *)&(ebpool->ebpool[out.eb].next));
  }
  return out;
}

void
update_edge_data (struct stinger * S, struct stinger_eb *eb,
                  uint64_t index, int64_t neighbor, int64_t weight, int64_t ts)
{
  struct stinger_edge * e = eb->edges + index;

  /* insertion */
  if(neighbor >= 0) {
    e->weight = weight;
    /* is this a new edge */
    if(e->neighbor < 0 || index >= eb->high) {
      e->neighbor = neighbor;

      /* only edge in block? - assuming we have block effectively locked */
      if(stinger_int64_fetch_add(&eb->numEdges, 1) == 0) {
	eb->smallStamp = ts;
	eb->largeStamp = ts;
      }

      /* register new edge */
      stinger_outdegree_increment_atomic(S, eb->vertexID, 1);
      stinger_indegree_increment_atomic(S, neighbor, 1);

      if (index >= eb->high)
	eb->high = index + 1;

      writexf(&e->timeFirst, ts);
    }

    /* check metadata and update - lock metadata for safety */
    if (ts < readff(&eb->smallStamp) || ts > eb->largeStamp) {
      int64_t smallStamp = readfe(&eb->smallStamp);
      if (ts < smallStamp)
	smallStamp = ts;
      if (ts > eb->largeStamp)
	eb->largeStamp = ts;
      writeef(&eb->smallStamp, smallStamp);
    }

    e->timeRecent = ts;

  } else if(e->neighbor >= 0) {
    /* are we deleting an edge */
    stinger_outdegree_increment_atomic(S, eb->vertexID, -1);
    stinger_indegree_increment_atomic(S, e->neighbor, -1);
    stinger_int64_fetch_add (&(eb->numEdges), -1);
    e->neighbor = neighbor;
  } 

  /* we always do this to update weight and  unlock the edge if needed */
}

/** @brief Insert a directed edge.
 *
 *  Inserts a typed, directed edge.  First timestamp is set, if the edge is
 *  new.  Recent timestamp is updated.  Weight is set to specified value regardless.
 *
 *  @param G The STINGER data structure
 *  @param type Edge type
 *  @param from Source vertex ID
 *  @param to Destination vertex ID
 *  @param weight Edge weight
 *  @param timestamp Edge timestamp
 *  @return 1 if edge is inserted successfully for the first time, 0 if edge is already found and updated, -1 if error.
 */
int
stinger_insert_edge (struct stinger *G,
                     int64_t type, int64_t from, int64_t to,
                     int64_t weight, int64_t timestamp)
{
  /* Do *NOT* call this concurrently with different edge types. */
  STINGERASSERTS ();
  MAP_STING(G);

  struct curs curs;
  struct stinger_eb *tmp;
  struct stinger_eb *ebpool_priv = ebpool->ebpool;

  curs = etype_begin (G, from, type);
  /*
  Possibilities:
  1: Edge already exists and only needs updated.
  2: Edge does not exist, fits in an existing block.
  3: Edge does not exist, needs a new block.
  */

  /* 1: Check if the edge already exists. */
  for (tmp = ebpool_priv + curs.eb; tmp != ebpool_priv; tmp = ebpool_priv + readff((uint64_t *)&tmp->next)) {
    if(type == tmp->etype) {
      size_t k, endk;
      endk = tmp->high;

      for (k = 0; k < endk; ++k) {
	if (to == tmp->edges[k].neighbor) {
	  update_edge_data (G, tmp, k, to, weight, timestamp);
	  return 0;
	}
      }
    }
  }

  while (1) {
    eb_index_t * block_ptr = curs.loc;
    curs.eb = readff((uint64_t *)curs.loc);
    /* 2: The edge isn't already there.  Check for an empty slot. */
    for (tmp = ebpool_priv + curs.eb; tmp != ebpool_priv; tmp = ebpool_priv + readff((uint64_t *)&tmp->next)) {
      if(type == tmp->etype) {
	size_t k, endk;
	endk = tmp->high;

	for (k = 0; k < STINGER_EDGEBLOCKSIZE; ++k) {
	  int64_t myNeighbor = tmp->edges[k].neighbor;
	  if (to == myNeighbor && k < endk) {
	    update_edge_data (G, tmp, k, to, weight, timestamp);
	    return 0;
	  }

	  if (myNeighbor < 0 || k >= endk) {
	    int64_t timefirst = readfe ( &(tmp->edges[k].timeFirst) );
	    int64_t thisEdge = tmp->edges[k].neighbor;
	    endk = tmp->high;

	    if (thisEdge < 0 || k >= endk) {
	      update_edge_data (G, tmp, k, to, weight, timestamp);
	      return 1;
	    } else if (to == thisEdge) {
	      update_edge_data (G, tmp, k, to, weight, timestamp);
	      writexf ( &(tmp->edges[k].timeFirst), timefirst);
	      return 0;
	    } else {
	      writexf ( &(tmp->edges[k].timeFirst), timefirst);
	    }
	  }
	}
      }
      block_ptr = &(tmp->next);
    }

    /* 3: Needs a new block to be inserted at end of list. */
    eb_index_t old_eb = readfe ((uint64_t *)block_ptr );
    if (!old_eb) {
      eb_index_t newBlock = new_eb (G, type, from);
      if (newBlock == 0) {
	writeef ((uint64_t *)block_ptr, (uint64_t)old_eb);
	return -1;
      } else {
	update_edge_data (G, ebpool_priv + newBlock, 0, to, weight, timestamp);
	ebpool_priv[newBlock].next = 0;
	push_ebs (G, 1, &newBlock);
      }
      writeef ((uint64_t *)block_ptr, (uint64_t)newBlock);
      return 1;
    }
    writeef ((uint64_t *)block_ptr, (uint64_t)old_eb);
  }
}

/** @brief Returns the out-degree of a vertex for a given edge type
 *
 *  @param S The STINGER data structure
 *  @param i Logical vertex ID
 *  @param type Edge type
 *  @return Out-degree of vertex i with type
 */
int64_t
stinger_typed_outdegree (const struct stinger * S, int64_t i, int64_t type) {
  MAP_STING(S);
  int64_t out = 0;
  struct curs curs;
  struct stinger_eb *tmp;
  struct stinger_eb *ebpool_priv = ebpool->ebpool;

  curs = etype_begin (S, i, type);

  for (tmp = ebpool_priv + curs.eb; tmp != ebpool_priv; tmp = ebpool_priv + readff((uint64_t *)&tmp->next)) {
    if(type == tmp->etype) {
      out++;
    }
  }
  return out;
}

/** @brief Increments a directed edge.
 *
 *  Increments the weight of a typed, directed edge.
 *  Recent timestamp is updated.
 *
 *  @param G The STINGER data structure
 *  @param type Edge type
 *  @param from Source vertex ID
 *  @param to Destination vertex ID
 *  @param weight Edge weight
 *  @param timestamp Edge timestamp
 *  @return 1
 */
int
stinger_incr_edge (struct stinger *G,
                   int64_t type, int64_t from, int64_t to,
                   int64_t weight, int64_t timestamp)
{
  /* Do *NOT* call this concurrently with different edge types. */
  STINGERASSERTS ();
  MAP_STING(G);

  struct curs curs;
  struct stinger_eb *tmp;
  struct stinger_eb *ebpool_priv = ebpool->ebpool;

  curs = etype_begin (G, from, type);
  /*
  Possibilities:
  1: Edge already exists and only needs updated.
  2: Edge does not exist, fits in an existing block.
  3: Edge does not exist, needs a new block.
  */

  /* 1: Check if the edge already exists. */
  for (tmp = ebpool_priv + curs.eb; tmp != ebpool_priv; tmp = ebpool_priv + readff((uint64_t *)&tmp->next)) {
    if(type == tmp->etype) {
      size_t k, endk;
      endk = tmp->high;

      for (k = 0; k < endk; ++k) {
	if (to == tmp->edges[k].neighbor) {
	  update_edge_data (G, tmp, k, to, tmp->edges[k].weight + weight, timestamp);
	  return 0;
	}
      }
    }
  }

  while (1) {
    eb_index_t * block_ptr = curs.loc;
    curs.eb = readff((uint64_t *)curs.loc);
    /* 2: The edge isn't already there.  Check for an empty slot. */
    for (tmp = ebpool_priv + curs.eb; tmp != ebpool_priv; tmp = ebpool_priv + readff((uint64_t *)&tmp->next)) {
      if(type == tmp->etype) {
	size_t k, endk;
	endk = tmp->high;

	for (k = 0; k < STINGER_EDGEBLOCKSIZE; ++k) {
	  int64_t myNeighbor = tmp->edges[k].neighbor;
	  if (to == myNeighbor && k < endk) {
	    update_edge_data (G, tmp, k, to, tmp->edges[k].weight + weight, timestamp);
	    return 0;
	  }

	  if (myNeighbor < 0 || k >= endk) {
	    int64_t timefirst = readfe ( &(tmp->edges[k].timeFirst) );
	    int64_t thisEdge = tmp->edges[k].neighbor;
	    endk = tmp->high;

	    if (thisEdge < 0 || k >= endk) {
	      update_edge_data (G, tmp, k, to, weight, timestamp);
	      return 1;
	    } else if (to == thisEdge) {
	      update_edge_data (G, tmp, k, to, tmp->edges[k].weight + weight, timestamp);
	      writexf ( &(tmp->edges[k].timeFirst), timefirst);
	      return 0;
	    } else {
	      writexf ( &(tmp->edges[k].timeFirst), timefirst);
	    }
	  }
	}
      }
      block_ptr = &(tmp->next);
    }

    /* 3: Needs a new block to be inserted at end of list. */
    eb_index_t old_eb = readfe ((uint64_t *)block_ptr );
    if (!old_eb) {
      eb_index_t newBlock = new_eb (G, type, from);
      if (newBlock == 0) {
	writeef ((uint64_t *)block_ptr, (uint64_t)old_eb);
	return -1;
      } else {
	update_edge_data (G, ebpool_priv + newBlock, 0, to, weight, timestamp);
	ebpool_priv[newBlock].next = 0;
	push_ebs (G, 1, &newBlock);
      }
      writeef ((uint64_t *)block_ptr, (uint64_t)newBlock);
      return 1;
    }
    writeef ((uint64_t *)block_ptr, (uint64_t)old_eb);
  }
}

/** @brief Insert an undirected edge.
 *
 *  Inserts a typed, undirected edge.  First timestamp is set, if the edge is
 *  new.  Recent timestamp is updated.  Weight is set to specified value regardless.
 *
 *  @param G The STINGER data structure
 *  @param type Edge type
 *  @param from Source vertex ID
 *  @param to Destination vertex ID
 *  @param weight Edge weight
 *  @param timestamp Edge timestamp
 *  @return Number of edges inserted successfully
 */
int
stinger_insert_edge_pair (struct stinger *G,
                          int64_t type, int64_t from, int64_t to,
                          int64_t weight, int64_t timestamp)
{
  STINGERASSERTS();

  int rtn = stinger_insert_edge (G, type, from, to, weight, timestamp);
  int rtn2 = stinger_insert_edge (G, type, to, from, weight, timestamp);

  /* Check if either returned -1 */
  if(rtn < 0 || rtn2 < 0)
    return -1;
  else
    return rtn | (rtn2 << 1);
}

/** @brief Increments an undirected edge.
 *
 *  Increments the weight of a typed, undirected edge.
 *  Recent timestamp is updated.
 *
 *  @param G The STINGER data structure
 *  @param type Edge type
 *  @param from Source vertex ID
 *  @param to Destination vertex ID
 *  @param weight Edge weight
 *  @param timestamp Edge timestamp
 *  @return 1
 */
int
stinger_incr_edge_pair (struct stinger *G,
                        int64_t type, int64_t from, int64_t to,
                        int64_t weight, int64_t timestamp)
{
  STINGERASSERTS();

  int rtn = stinger_incr_edge (G, type, from, to, weight, timestamp);
  int rtn2 = stinger_incr_edge (G, type, to, from, weight, timestamp);

  /* Check if either returned -1 */
  if(rtn < 0 || rtn2 < 0)
    return -1;
  else
    return rtn | (rtn2 << 1);
}

/** @brief Removes a directed edge.
 *
 *  Remove a typed, directed edge.
 *  Note: Do not call this function concurrently with the same source vertex,
 *  even for different edge types.
 *
 *  @param G The STINGER data structure
 *  @param type Edge type
 *  @param from Source vertex ID
 *  @param to Destination vertex ID
 *  @return 1 on success, 0 if the edge is not found.
 */
int
stinger_remove_edge (struct stinger *G,
                     int64_t type, int64_t from, int64_t to)
{
  /* Do *NOT* call this concurrently with different edge types. */
  STINGERASSERTS ();
  MAP_STING(G);

  struct curs curs;
  struct stinger_eb *tmp;
  struct stinger_eb *ebpool_priv = ebpool->ebpool;

  curs = etype_begin (G, from, type);

  for (tmp = ebpool_priv + curs.eb; tmp != ebpool_priv; tmp = ebpool_priv + readff((uint64_t *)&tmp->next)) {
    if(type == tmp->etype) {
      size_t k, endk;
      endk = tmp->high;

      for (k = 0; k < endk; ++k) {
	if (to == tmp->edges[k].neighbor) {
	  int64_t weight = readfe (&(tmp->edges[k].weight));
	  if(to == tmp->edges[k].neighbor) {
	    update_edge_data (G, tmp, k, ~to, weight, 0);
	    return 1;
	  } else {
	    writeef((uint64_t *)&(tmp->edges[k].weight), (uint64_t)weight);
	  }
	  return 0;
	}
      }
    }
  }
  return -1;
}

/** @brief Removes an undirected edge.
 *
 *  Remove a typed, undirected edge.
 *  Note: Do not call this function concurrently with the same source vertex,
 *  even for different edge types.
 *
 *  @param G The STINGER data structure
 *  @param type Edge type
 *  @param from Source vertex ID
 *  @param to Destination vertex ID
 *  @return 1 on success, 0 if the edge is not found.
 */
int
stinger_remove_edge_pair (struct stinger *G,
                          int64_t type, int64_t from, int64_t to)
{
  STINGERASSERTS();

  int rtn = stinger_remove_edge (G, type, from, to);
  int rtn2 = stinger_remove_edge (G, type, to, from);

  /* Check if either returned -1 */
  if(rtn < 0 || rtn2 < 0)
    return -1;
  else
    return rtn | (rtn2 << 1);
}

/** @brief Get the weight of a given edge.
 *
 *  Finds a specified edge of a given type by source and destination vertex ID
 *  and returns the current edge weight.  Remember, edges may have different
 *  weights in different directions.
 *
 *  @param G The STINGER data structure
 *  @param from Source vertex ID
 *  @param to Destination vertex ID
 *  @param type Edge type
 *  @return Edge weight
 */
int64_t
stinger_edgeweight (const struct stinger * G,
                    int64_t from, int64_t to, int64_t type)
{
  STINGERASSERTS ();

  int rtn = 0;

  STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(G,type,from) {
    if (STINGER_EDGE_DEST == to) {
      rtn = STINGER_EDGE_WEIGHT;
    }
  } STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_END();
  return rtn;
}

/* TODO revisit this function
 * XXX how to handle in parallel?
 * BUG block meta not handled
 * */
/** @brief Set the weight of a given edge.
 *
 *  Finds a specified edge of a given type by source and destination vertex ID
 *  and sets the current edge weight.  Remember, edges may have different
 *  weights in different directions.
 *
 *  @param G The STINGER data structure
 *  @param from Source vertex ID
 *  @param to Destination vertex ID
 *  @param type Edge type
 *  @param weight Edge weight to set
 *  @return 1 on success; 0 otherwise
 */
int
stinger_set_edgeweight (struct stinger *G,
                        int64_t from, int64_t to, int64_t type,
                        int64_t weight)
{
  STINGERASSERTS ();

  int rtn = 0;

  STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(G,type,from) {
    if (STINGER_EDGE_DEST == to) {
      STINGER_EDGE_WEIGHT = weight;
      rtn = 1;
    }
  } STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_END();
  return rtn;
}

/** @brief Get the first timestamp of a given edge.
 *
 *  Finds a specified edge of a given type by source and destination vertex ID
 *  and returns the first timestamp.  Remember, edges may have different
 *  timestamps in different directions.
 *
 *  @param G The STINGER data structure
 *  @param from Source vertex ID
 *  @param to Destination vertex ID
 *  @param type Edge type
 *  @return First timestamp of edge; -1 if edge does not exist
 */
int64_t
stinger_edge_timestamp_first (const struct stinger * G,
                              int64_t from, int64_t to, int64_t type)
{
  STINGERASSERTS ();

  int rtn = -1;

  STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(G,type,from) {
    if (STINGER_EDGE_DEST == to) {
      rtn = STINGER_EDGE_TIME_FIRST;
    }
  } STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_END();
  return rtn;
}

/** @brief Get the recent timestamp of a given edge.
 *
 *  Finds a specified edge of a given type by source and destination vertex ID
 *  and returns the recent timestamp.  Remember, edges may have different
 *  timestamps in different directions.
 *
 *  @param G The STINGER data structure
 *  @param from Source vertex ID
 *  @param to Destination vertex ID
 *  @param type Edge type
 *  @return Recent timestamp of edge; -1 if edge does not exist
 */
int64_t
stinger_edge_timestamp_recent (const struct stinger * G,
                               int64_t from, int64_t to, int64_t type)
{
  STINGERASSERTS ();

  int rtn = -1;

  STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(G,type,from) {
    if (STINGER_EDGE_DEST == to) {
      rtn = STINGER_EDGE_TIME_RECENT;
    }
  } STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_END();
  return rtn;
}

/* TODO revisit this function
 * XXX how to handle in parallel?
 * BUG block meta not handled
 * */
/** @brief Update the recent timestamp of an edge
 *
 *  Finds a specified edge of a given type by source and destination vertex ID
 *  and updates the recent timestamp to the specified value.
 *
 *  @param G The STINGER data structure
 *  @param from Source vertex ID
 *  @param to Destination vertex ID
 *  @param type Edge type
 *  @param timestamp Timestamp to set recent
 *  @return 1 on success; 0 if failure
 */
int
stinger_edge_touch (struct stinger *G,
    int64_t from, int64_t to, int64_t type,
    int64_t timestamp)
{
  STINGERASSERTS ();

  int rtn = 0;

  STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(G,type,from) {
    if (STINGER_EDGE_DEST == to) {
      STINGER_EDGE_TIME_RECENT = timestamp;
      rtn = 1;
    }
  } STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_END();
  return rtn;
}

/* TODO - add doxygen and insert into stinger.h */
int64_t
stinger_count_outdeg (struct stinger * G, int64_t v)
{
  int64_t nactive_edges = 0, neb = 0;

  STINGER_FORALL_EB_BEGIN (G, v, eb) {
    const size_t eblen = stinger_eb_high (eb);
    OMP ("omp parallel for")
      for (size_t ek = 0; ek < eblen; ++ek) {
        if (!stinger_eb_is_blank (eb, ek))
          stinger_int64_fetch_add (&nactive_edges, 1);
      }
  }
  STINGER_FORALL_EB_END ();

  return nactive_edges;
}

int
stinger_delete_vertex (struct stinger *S, int64_t vtx)
{
  int64_t returnCode = 0;

  STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, vtx) {
    stinger_remove_edge_pair (S, STINGER_EDGE_TYPE, STINGER_EDGE_DEST, vtx);
  } STINGER_FORALL_EDGES_OF_VTX_END();

  stinger_vweight_set (S, vtx, 0);

  return returnCode;
}

/**
* @brief Calculate statistics on edge block fragmentation in the graph
*
* @param S The STINGER data structure
* @param NV The total number of vertices
*
* @return Void
*/
void
stinger_fragmentation (struct stinger *S, uint64_t NV, struct stinger_fragmentation_t * stats)
{
  uint64_t numSpaces = 0;
  uint64_t numBlocks = 0;
  uint64_t numEdges = 0;
  uint64_t numEmptyBlocks = 0;

  MAP_STING(S);
  struct stinger_eb * ebpool_priv = ebpool->ebpool;
  OMP ("omp parallel for reduction(+:numSpaces, numBlocks, numEdges, numEmptyBlocks)")
  for (uint64_t i = 0; i < NV; i++) {
    const struct stinger_eb *curBlock = ebpool_priv + stinger_vertex_edges_get(vertices, i);

    while (curBlock != ebpool_priv) {
      uint64_t found = 0;

      if (curBlock->numEdges == 0) {
	numEmptyBlocks++;
      }
      else {
	/* for each edge in the current block */
	for (uint64_t j = 0; j < curBlock->high && j < STINGER_EDGEBLOCKSIZE; j++) {
	  if (stinger_eb_is_blank (curBlock, j)) {
	    numSpaces++;
	    found = 1;
	  }
	  else {
	    numEdges++;
	  }
	}
      }

      numBlocks += found;
      curBlock = ebpool_priv + curBlock->next;
    }
  }

  int64_t totalEdgeBlocks = ETA->high;

  stats->num_empty_edges = numSpaces;
  stats->num_fragmented_blocks = numBlocks;
  stats->num_edges = numEdges;
  stats->edge_blocks_in_use = totalEdgeBlocks;
  stats->avg_number_of_edges = (double) numEdges / (double) (totalEdgeBlocks-numEmptyBlocks);
  stats->num_empty_blocks = numEmptyBlocks;

  double fillPercent = (double) numEdges / (double) (totalEdgeBlocks * STINGER_EDGEBLOCKSIZE);
  stats->fill_percent = fillPercent;
}
