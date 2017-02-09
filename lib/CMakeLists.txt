add_subdirectory(dynograph_util)
link_libraries(hooks)
include_directories(${CMAKE_SOURCE_DIR}/lib/dynograph_util/hooks)
add_subdirectory(stinger)