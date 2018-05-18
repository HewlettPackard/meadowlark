cmake_minimum_required(VERSION 2.8.11)
project(YCSBC)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

#
# default system include and link directories
#
link_directories(/usr/lib)
include_directories(/usr/include)
link_directories(/usr/local/lib)
include_directories(/usr/local/include)

#
# build type: Release (default) or Debug
#
message(STATUS "====================")
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()
if(CMAKE_BUILD_TYPE MATCHES Debug)
  message(STATUS "Build type: Debug")
elseif(CMAKE_BUILD_TYPE MATCHES Release)
  message(STATUS "Build type: Release")
else()
  message(STATUS "Invalid build type: ${CMAKE_BUILD_TYPE}")
  message(STATUS "Default to : Release")
  set(CMAKE_BUILD_TYPE "Release")
endif()

#
# use the FAM version of CounterGenerator (for key generation)
#
if(FAME) # FAME
  add_definitions(-DFAME)
  add_definitions(-DFAM_ATOMIC)
  message(STATUS "atomics library: FAM")
  message(STATUS "Store the CounterGenerator in FAM")
else() # NUMA
  message(STATUS "atomics library: native")
  message(STATUS "Store the CounterGenerator in local DRAM")
endif()

message(STATUS "====================")


#
# common gcc flags
#
set(CMAKE_CXX_FLAGS "-std=c++11")
set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} -Wall -W -Wextra -Wredundant-decls -Wunused -Wunused-macros -Wno-unused-parameter -Wcast-qual -Wcast-align -Wwrite-strings -Wmissing-field-initializers -Wendif-labels -Winit-self -Wlogical-op -Wpacked -Wstack-protector -Wformat=2 -Wswitch-enum -Wstrict-overflow=5 -Wpointer-arith -Wnormalized=nfc -Wno-long-long -Wswitch-default -Wconversion -Wunreachable-code")  

# Flags that are currently not used: -Wshadow -Wpadded
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-offsetof ") # to suppress warnings about offsetof

#
# gcc flags for release build
#
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -g -O2")

#
# gcc flags for debug build
#
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -DDEBUG -Og -g")

#
# include directories
#
include_directories(${PROJECT_SOURCE_DIR}/src) # private headers (src)

#
# header-files directory and library for nvmm and kvs
#
find_package(NVMM REQUIRED)
include_directories(${NVMM_INCLUDE_DIR})
find_package(RadixTree REQUIRED)
include_directories(${RADIXTREE_INCLUDE_DIR})
find_package(KVS_Client REQUIRED)
include_directories(${KVS_CLIENT_INCLUDE_DIR})
find_package(KVS_Cache REQUIRED)
include_directories(${KVS_CACHE_INCLUDE_DIR})

#
# third-party libraries
#
include_directories(${PROJECT_SOURCE_DIR}/third-party/cereal/include)

#
# add source code
#
add_subdirectory(src)
add_subdirectory(bin)
