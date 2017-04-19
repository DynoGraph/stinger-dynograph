// Provides a C API for stinger extensions implemented in stinger-dynograph

#ifndef STINGER_DYNOGRAPH_UTILS_H
#define STINGER_DYNOGRAPH_UTILS_H

// Initialize a stinger graph from an edge list
//   Arguments:
//     S     - pointer to pre-initialized stinger structure
//     edges - pointer to an array of edges (of type DynoGraph::Edge)
//     n     - length of edge array
//   Notes:
//     - Calls stinger_set_initial_edges(),
//     - Use only on an empty stinger graph
//     - The edge array must not have any duplicates
//     - The edge array does not have to be sorted
//
void dynograph_init_stinger_from_edge_list(stinger_t *S, void * edges, size_t n);

#endif //STINGER_DYNOGRAPH_UTILS_H
