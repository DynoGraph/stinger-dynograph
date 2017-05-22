set(CMAKE_C_COMPILER "$ENV{CONTECH_HOME}/scripts/contech_wrapper.py")
set(CMAKE_CXX_COMPILER "$ENV{CONTECH_HOME}/scripts/contech_wrapper++.py")

# These get overwritten during config, so we need to set the command instead
# Perhaps if we point cmake at $ENV{CONTECH_LLVM_HOME}/bin for all executables, it will work
# But for now it needs to find contech_wrapper instead of clang
#    set(CMAKE_AR "$ENV{CONTECH_LLVM_HOME}/bin/llvm-ar")
#    set(CMAKE_RANLIB "$ENV{CONTECH_LLVM_HOME}/bin/llvm-ranlib")

set(CMAKE_C_ARCHIVE_CREATE "$ENV{CONTECH_LLVM_HOME}/bin/llvm-ar rcs <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_CXX_ARCHIVE_CREATE "${CMAKE_C_ARCHIVE_CREATE}")
set(CMAKE_C_ARCHIVE_FINISH "$ENV{CONTECH_LLVM_HOME}/bin/llvm-ranlib <TARGET>")
set(CMAKE_CXX_ARCHIVE_FINISH "${CMAKE_C_ARCHIVE_FINISH}")

# Not sure if I can set these here...
#set(OpenMP_C_FLAGS "-fopenmp -I/home/ehein6/sources/libomp_oss/build/src -L/home/ehein6/sources/libomp_oss/build/src -liomp5")
#set(OpenMP_CXX_FLAGS "${OpenMP_C_FLAGS}")
