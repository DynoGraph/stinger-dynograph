# Try to find PERFMON headers and libraries.
#
# Usage of this module as follows:
#
#     find_package(PERFMON)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  PERFMON_PREFIX         Set this variable to the root installation of
#                      libpfm if the module has problems finding the
#                      proper installation path.
#
# Variables defined by this module:
#
#  PERFMON_FOUND              System has perfmon libraries and headers
#  PERFMON_LIBRARIES          The libpfm library
#  PERFMON_INCLUDE_DIRS       The location of libpfm headers

find_path(PERFMON_PREFIX
    NAMES include/perfmon/pfmlib.h
)

find_library(PERFMON_LIBRARIES
    # Pick the static library first for easier run-time linking.
    NAMES libpfm.a pfm
    HINTS ${PERFMON_PREFIX}/lib
)

find_path(PERFMON_INCLUDE_DIRS
    NAMES pfmlib.h
    HINTS ${PERFMON_PREFIX}/include/perfmon
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PERFMON DEFAULT_MSG
    PERFMON_LIBRARIES
    PERFMON_INCLUDE_DIRS
)

mark_as_advanced(
    PERFMON_PREFIX_DIRS
    PERFMON_LIBRARIES
    PERFMON_INCLUDE_DIRS
)

