set(sources

# FIXME move these to subdir
  src/community_on_demand.c
  src/shortest_paths.cpp
  src/diameter.cpp
  src/independent_sets.c
  src/graph_partition.c
  src/hits_centrality.c
  src/diameter.cpp
  src/independent_sets.c
  src/graph_partition.c
  src/hits_centrality.c
  src/modularity.c
  src/spmv_spmspv.c
  src/pagerank_updating.c

  adamic_adar/src/adamic_adar.c

  betweenness/src/betweenness.c
  betweenness/src/dynamic_betweenness.cpp

  bfs/src/bfs.c
  bfs/src/dynamic_bfs.cpp

  clustering/src/clustering.c
  clustering/src/dynamic_clustering.cpp

  kcore/src/kcore.c
  kcore/src/dynamic_kcore.cpp

  pagerank/src/pagerank.c
  pagerank/src/dynamic_pagerank.cpp

  random/src/random.c

  rmat/src/rmat.c

  simple_communities/src/community.c
  simple_communities/src/community-update.c
  simple_communities/src/graph-el.c
  simple_communities/src/sorts.c
  simple_communities/src/dynamic_simple_communities.cpp
  simple_communities/src/dynamic_simple_communities_updating.cpp

  static_components/src/static_components.c
  static_components/src/dynamic_static_components.cpp

  streaming_connected_components/src/streaming_connected_components.c
  streaming_connected_components/src/dynamic_streaming_connected_components.cpp

  weakly_connected_components/src/weakly_connected_components.c
)
# Don't include all .h files here, just the ones that contain public definitions
set(headers
# FIXME move these to subdir
  inc/community_on_demand.h
  inc/shortest_paths.h
  inc/diameter.h
  inc/independent_sets.h
  inc/graph_partition.h
  inc/hits_centrality.h
  inc/modularity.h
  inc/spmv_spmspv.h
  inc/pagerank_updating.h

  adamic_adar/inc/adamic_adar.h
  betweenness/inc/betweenness.h
  betweenness/inc/dynamic_betweenness.h

  bfs/inc/bfs.h
  bfs/inc/dynamic_bfs.h

  clustering/inc/clustering.h
  clustering/inc/dynamic_clustering.h

  kcore/inc/kcore.h
  kcore/inc/dynamic_kcore.h

  pagerank/inc/pagerank.h
  pagerank/inc/dynamic_pagerank.h

  random/inc/random.h
  rmat/inc/rmat.h

  simple_communities/inc/community.h
  simple_communities/inc/defs.h
  simple_communities/inc/graph-el.h
  simple_communities/inc/community-update.h
  simple_communities/inc/dynamic_simple_communities.h
  simple_communities/inc/dynamic_simple_communities_updating.h

  static_components/inc/static_components.h
  static_components/inc/dynamic_static_components.h

  streaming_connected_components/inc/streaming_connected_components.h
  streaming_connected_components/inc/dynamic_streaming_connected_components.h

  weakly_connected_components/inc/weakly_connected_components.h

  common/inc/streaming_algorithm.h
)

publish_headers(headers "${CMAKE_BINARY_DIR}/include/stinger_alg")

include_directories("${CMAKE_BINARY_DIR}/include/stinger_alg")
include_directories("${CMAKE_BINARY_DIR}/include/stinger_core")
include_directories("${CMAKE_BINARY_DIR}/include/stinger_net")

set_source_files_properties(${config} PROPERTIES GENERATED TRUE)
add_library(stinger_alg ${sources} ${headers} ${config})
target_link_libraries(stinger_alg stinger_core compat)
