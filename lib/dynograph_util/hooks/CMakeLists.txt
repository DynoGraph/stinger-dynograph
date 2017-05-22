cmake_minimum_required (VERSION 2.8.11)
project (HOOKS)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(HOOKS_PRETTY_PRINT FALSE CACHE BOOL "Print formatted JSON to stdout, instead of all on one line")
set(HOOKS_TYPE "" CACHE STRING "Select type of hooks to add. Values are 'NONE', 'GEM5', 'SNIPER', 'PIN', 'PAPI' ")

if(${HOOKS_PRETTY_PRINT})
	add_definitions(-DHOOKS_PRETTY_PRINT)
endif()

if (HOOKS_TYPE STREQUAL "")
	# No hooks
elseif (HOOKS_TYPE STREQUAL "SNIPER")
	add_definitions(-DENABLE_SNIPER_HOOKS)
	set(SNIPER_ROOT $ENV{SNIPER_ROOT})
	set(GRAPHITE_ROOT $ENV{SNIPER_ROOT})
	include($ENV{SNIPER_ROOT}/config/buildconf.cmake)
	set(BENCHMARKS_ROOT $ENV{BENCHMARKS_ROOT})
	include($ENV{BENCHMARKS_ROOT}/tools/hooks/buildconf.cmake)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SNIPER_CFLAGS} ${HOOKS_CFLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SNIPER_LDFLAGS} ${HOOKS_LDFLAGS}")

elseif (HOOKS_TYPE STREQUAL "GEM5")
	add_definitions(-DENABLE_GEM5_HOOKS)
	set(GEM5_HOME $ENV{GEM5_HOME})
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static -I${GEM5_HOME}")
	if (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GEM5_HOME}/util/m5/m5op_arm_A64.S")
	else()
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GEM5_HOME}/util/m5/m5op_${CMAKE_SYSTEM_PROCESSOR}.S")
	endif()

elseif (HOOKS_TYPE STREQUAL "PIN")
	add_definitions(-DENABLE_PIN_HOOKS)

elseif (HOOKS_TYPE STREQUAL "PERF")
	add_definitions(-DENABLE_PERF_HOOKS)
	find_package( PERFMON REQUIRED )
	include_directories(${PERFMON_INCLUDE_DIRS})
	# pfm_cxx provides a c++ wrapper to the perfmon interface
	include_directories("perf")
	add_library(pfm_cxx perf/pfm_cxx.cpp perf/pfm_cxx.h)
	set(HOOKS_LIBS "${HOOKS_LIBS};pfm_cxx;${PERFMON_LIBRARIES}")

else ()
	message(FATAL_ERROR "Invalid value for HOOKS_TYPE : ${HOOKS_TYPE}")
endif()

# Use the gnu9x C standard
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu9x")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_library(hooks hooks.cc edge_count.c)
target_link_libraries(hooks ${HOOKS_LIBS})
