cmake_minimum_required (VERSION 2.8.11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_subdirectory(hooks)

# Build the dynograph_util library
add_library(dynograph_util
    args.cc
    alg_data_manager.cc
    batch.cc
    benchmark.cc
    edgelist_dataset.cc
    rmat_dataset.cc
)
# Enable parallel versions of functions from <algorithm> and <numeric>
if (OPENMP_FOUND)
  target_compile_definitions(dynograph_util PUBLIC _GLIBCXX_PARALLEL)
endif()
target_link_libraries(dynograph_util hooks)
target_include_directories(dynograph_util PUBLIC hooks)

# Build the RMAT graph dumper
add_executable(rmat_dataset_dump rmat_dataset_dump.cc)
target_link_libraries(rmat_dataset_dump dynograph_util)

# Build the bin_to_el utility
add_executable(bin_to_el bin_to_el.cc)
target_link_libraries(bin_to_el dynograph_util)

# Detect if googletest was already built elsewhere
if (NOT GOOGLETEST_DIR)
  set(GOOGLETEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/googletest/include PARENT_SCOPE)
  add_subdirectory(googletest)
endif()

enable_testing()

# Build the unit tests
function(add_test_exe name)
  add_executable(${name} ${name}.cc)
  target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${GOOGLETEST_DIR})
  target_link_libraries(${name} dynograph_util gtest_main)
  if (ARGC EQUAL 1)
    add_test(${name} ${name})
  else()
    add_test(${name} ${name} ${ARGV1})
  endif()
endfunction()

add_test_exe(dynograph_util_test ${CMAKE_CURRENT_SOURCE_DIR}/data/ring-of-cliques.graph.el)
add_test_exe(reference_impl_test)
add_test_exe(rmat_dataset_test)
add_test_exe(batch_test)

# Copy test data to the build directory
file(
    COPY
    data/ring-of-cliques.graph.bin
    data/worldcup-10K.graph.bin
    DESTINATION
    data/
)
