include_directories("${CMAKE_BINARY_DIR}/include/stinger_alg")
include_directories("${CMAKE_BINARY_DIR}/include/stinger_core")
include_directories("${CMAKE_BINARY_DIR}/include/stinger_bench")

add_executable(converter converter.c)
target_link_libraries(converter stinger_core stinger_alg)
