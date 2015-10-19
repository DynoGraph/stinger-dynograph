cmake_minimum_required (VERSION 2.8.11)
project (DYNOGRAPH)

# Define a single common include directory
# All include files should be copied here
include_directories("${CMAKE_BINARY_DIR}/include")

# Build with OpenMP
find_package( OpenMP )
if(OPENMP_FOUND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

# Build static libraries
option(BUILD_SHARED_LIBS "Build libraries as SHARED" OFF)

# Use the gnu9x C standard
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu9x")

# Begin building subdirectories
add_subdirectory(lib)
# add_subdirectory(src)