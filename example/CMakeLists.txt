cmake_minimum_required( VERSION 3.21 )

project( memfog-example VERSION 1.0.0 LANGUAGES CXX )

set( CMAKE_CXX_STANDARD 23 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

# output to build/Debug or build/Release
set( BUILD_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/build/$<CONFIG> )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BUILD_OUTPUT_DIR} )

add_executable( memfog-example main.cxx )

# include parent dir so #include <memfog/memfog.hxx> works
target_include_directories( memfog-example PRIVATE ${CMAKE_SOURCE_DIR}/.. )
