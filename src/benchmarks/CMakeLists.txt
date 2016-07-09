
file(COPY ${headers} DESTINATION "${CMAKE_BINARY_DIR}/include/dynograph")

include_directories("${CMAKE_BINARY_DIR}/include/stinger_alg")
include_directories("${CMAKE_BINARY_DIR}/include/stinger_core")
include_directories("${CMAKE_BINARY_DIR}/include/dynograph")
include_directories("${CMAKE_SOURCE_DIR}/lib/dynograph_util")
include_directories("${CMAKE_SOURCE_DIR}/lib/hooks")

add_executable(dynograph src/dynograph.cpp)
target_link_libraries(dynograph stinger_core stinger_alg dynograph_util hooks)
