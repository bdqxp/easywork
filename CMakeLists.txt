cmake_minimum_required(VERSION 2.8)
project(easywork)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_BUILD_TYPE Debug)
set(BOOST_HEADERS /usr/local/include/boost)
set(BOOST_LIBS /usr/local/lib64/boost)
set(LIBEV_HEADERS /usr/local/include/libev)
set(LIBEV_LIBS /usr/local/lib64/libev)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

add_definitions(-Wall)
# add_definitions(-Wall -Wextra)
add_definitions(-D_GNU_SOURCE)
add_definitions(-ggdb -O0 -pipe -rdynamic)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(-D_DEBUG)
endif()

include_directories(${BOOST_HEADERS})
include_directories(${LIBEV_HEADERS})
include_directories(${CMAKE_SOURCE_DIR}/include)

link_directories(${BOOST_LIBS})
link_directories(${LIBEV_LIBS})
link_directories(${CMAKE_SOURCE_DIR}/lib)

add_subdirectory(src)
