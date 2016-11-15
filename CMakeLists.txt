cmake_minimum_required (VERSION 2.8.11)
project (DYNOGRAPH)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

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

add_subdirectory(lib)

add_executable(dynograph dynograph.cpp)
target_include_directories(dynograph PRIVATE
  ${CMAKE_BINARY_DIR}/include # STINGER publishes headers to build directory
  lib/hooks
  lib/dynograph_util
)
target_link_libraries(dynograph stinger_core stinger_alg dynograph_util hooks)
