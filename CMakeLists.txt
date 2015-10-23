cmake_minimum_required (VERSION 2.8.11)
project (DYNOGRAPH)

set(HOOKS_TYPE "NONE" CACHE STRING "Add flags and/or ROI hooks for running under a simulator. Values are 'GEM5', 'SNIPER', 'PIN', 'PAPI' ")

if (HOOKS_TYPE EQUAL "SNIPER")
	add_definitions(-DENABLE_SNIPER_HOOKS)
	include($ENV{SNIPER_ROOT}/config/buildconf.cmake)
	include($ENV{BENCHMARKS_ROOT}/tools/hooks/buildconf.cmake)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${HOOKS_CFLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SNIPER_LDFLAGS} ${HOOKS_LDFLAGS}")

elseif (HOOKS_TYPE EQUAL "GEM5")
	add_definitions(-DENABLE_GEM5_HOOKS)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static -I${GEM5_HOME}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} $(GEM5_HOME)/util/m5/m5op_alpha.S")

elseif (HOOKS_TYPE EQUAL "PIN")
	add_definitions(-DENABLE_PIN_HOOKS)

endif()
# TODO make sure only one of the above is ever set

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

include_directories("${CMAKE_BINARY_DIR}/include")

link_libraries(m)

# Build with OpenMP
find_package( OpenMP )
if(OPENMP_FOUND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

# Use the gnu9x C standard
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu9x")

# Memory size configuration bits for stinger_core
set(STINGER_DEFAULT_VERTICES "(1L<<24)" CACHE STRING "Default number of vertices")
set(STINGER_DEFAULT_NUMETYPES "5" CACHE STRING "Default number of edge types")
set(STINGER_DEFAULT_NUMVTYPES "128" CACHE STRING "Default number of vertex types")
set(STINGER_DEFAULT_NEB_FACTOR "4" CACHE STRING "Default number of edge blocks per vertex")
set(STINGER_EDGEBLOCKSIZE "14" CACHE STRING "Number of edges per edge block")
set(STINGER_NAME_STR_MAX "255" CACHE STRING "Max string length in physmap")

configure_file(${CMAKE_SOURCE_DIR}/lib/stinger_core/inc/stinger_defs.h.in ${CMAKE_BINARY_DIR}/include/stinger_core/stinger_defs.h @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/lib/stinger_core/inc/stinger_names.h.in ${CMAKE_BINARY_DIR}/include/stinger_core/stinger_names.h @ONLY)


# Begin building subdirectories
add_subdirectory(lib)
add_subdirectory(src)