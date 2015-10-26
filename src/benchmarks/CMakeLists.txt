set(headers
  inc/stinger_bench.h
  inc/hooks.h
)

file(COPY ${headers} DESTINATION "${CMAKE_BINARY_DIR}/include/benchmarks")

include_directories("${CMAKE_BINARY_DIR}/include/stinger_alg")
include_directories("${CMAKE_BINARY_DIR}/include/stinger_core")
include_directories("${CMAKE_BINARY_DIR}/include/benchmarks")

macro(add_benchmark bench_name)
  add_executable(${bench_name} src/${bench_name}.c src/stinger_bench.c src/hooks.c)
  target_link_libraries(${bench_name} stinger_core stinger_alg ${BENCHMARK_LIBS})
endmacro()

add_benchmark(adamic_adar)
add_benchmark(betweenness)
add_benchmark(bfs)
add_benchmark(bfs_direction_optimizing)
add_benchmark(components)
add_benchmark(clustering)
add_benchmark(kcore)
add_benchmark(pagerank)