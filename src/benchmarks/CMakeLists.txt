set(headers
  inc/dynograph_util.h
  inc/hooks.h
  inc/perf.h
  inc/pfm_cxx.h
)

file(COPY ${headers} DESTINATION "${CMAKE_BINARY_DIR}/include/dynograph")

include_directories("${CMAKE_BINARY_DIR}/include/stinger_alg")
include_directories("${CMAKE_BINARY_DIR}/include/stinger_core")
include_directories("${CMAKE_BINARY_DIR}/include/dynograph")
if (HOOKS_TYPE STREQUAL "PERF")
	add_library(pfm_cxx STATIC src/pfm_cxx.cpp inc/pfm_cxx.h)
endif()
add_executable(dynograph src/dynograph.cpp src/dynograph_util.c src/hooks.cpp)
target_link_libraries(dynograph stinger_core stinger_alg ${BENCHMARK_LIBS})
