#
# CMakeLists.txt  cmake for hex-runner
#

#
# Use CMAKE_INSTALL_PATH or CMAKE_PREFIX_PATH to specify custom lib locations
#
cmake_minimum_required(VERSION 3.1)

project (hex-runner C CXX)

set (CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set (CMAKE_CXX_STANDARD 11)

include (CheckCXXCompilerFlag)
check_cxx_compiler_flag (-Wall flagWall)
if (flagWall)
    add_compile_options (-Wall)
endif ()

add_executable (hex-runner hex-runner.cc)

#
# "make install" rule
#
install (TARGETS hex-runner RUNTIME DESTINATION bin)
