cmake_minimum_required(VERSION 2.8.8)

#######################
# project information #
#######################

project(ted)
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)

include(${CMAKE_SOURCE_DIR}/cmake/include/define_module.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/include/git_sha1.cmake)

#####################
# compiler settings #
#####################

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -fomit-frame-pointer -fPIC -std=c++11 -DWITH_BOOST_GRAPH")
set(CMAKE_CXX_FLAGS_DEBUG   "-g -Wall -Wextra -fPIC -std=c++11 -DWITH_BOOST_GRAPH")
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Release or Debug" FORCE)
endif()

#######################
# project directories #
#######################

include_directories(${PROJECT_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR})

add_subdirectory(modules)
add_subdirectory(inference)
add_subdirectory(evaluation)
add_subdirectory(binaries)
add_subdirectory(python)

###############
# config file #
###############

configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/config.h.in
  ${PROJECT_BINARY_DIR}/config.h
)
