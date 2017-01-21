cmake_minimum_required (VERSION 2.8.11)
project (stinger-dynograph)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(BUILD_SHARED_LIBS OFF)
set(USE_STINGER_BATCH_INSERT TRUE CACHE BOOL "Use the new stinger_batch functions to do insertions.")
if(${USE_STINGER_BATCH_INSERT})
  add_definitions(-DUSE_STINGER_BATCH_INSERT)
endif()

# Build with OpenMP
find_package( OpenMP )
if(OPENMP_FOUND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

# Use the gnu9x C standard
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu9x")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Inform dynograph_util that googletest already exists in stinger
set(GOOGLETEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/stinger/external/googletest/include)

add_subdirectory(lib)

include_directories(
  ${CMAKE_BINARY_DIR}/include # STINGER publishes headers to build directory
  lib/dynograph_util
)

link_libraries(
  stinger_core
  stinger_alg
  dynograph_util
  hooks
)

add_executable(dynograph
  main.cpp
  stinger_server.cpp
  stinger_algorithm.cpp
  stinger_graph.cpp
)

# Tests

add_executable(stinger_dynograph_test
  stinger_dynograph_test.cc
  stinger_server.cpp
  stinger_algorithm.cpp
  stinger_graph.cpp
)
target_include_directories(stinger_dynograph_test PRIVATE ${GOOGLETEST_DIR} dynograph_util)
target_link_libraries(stinger_dynograph_test dynograph_util gtest_main)

enable_testing()
add_test(NAME stinger_dynograph_test COMMAND stinger_dynograph_test WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
