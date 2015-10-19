set(sources
  src/pagerank.c
  src/hooks.c
  src/stinger_bench.c
)
set(headers
  inc/stinger_bench.h
  inc/hooks.h
)

file(COPY ${headers} DESTINATION "${CMAKE_BINARY_DIR}/include/benchmarks")

include_directories("${CMAKE_BINARY_DIR}/include/stinger_alg")
include_directories("${CMAKE_BINARY_DIR}/include/stinger_core")
include_directories("${CMAKE_BINARY_DIR}/include/benchmarks")

set_source_files_properties(${config} PROPERTIES GENERATED TRUE)

add_executable(pagerank src/pagerank.c src/stinger_bench.c src/hooks.c)
target_link_libraries(pagerank stinger_core stinger_alg)
