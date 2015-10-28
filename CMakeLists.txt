cmake_minimum_required(VERSION 2.8)
project(bp)

#set(CMAKE_C_COMPILER clang)
#plugin source files
set(PLUGIN_SOURCE perf.c)

#additional c flags
set(CMAKE_C_FLAGS "-D_GNU_SOURCE -std=c11")

#debugging c flags
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DHAVE_DEBUG -O0 -Wstrict-prototypes -Wall -Wundef -Wno-long-long -Wsign-compare -Wcomment -pedantic -finline-functions -fno-strict-aliasing")
set(CMAKE_C_FLAGS_RELEASE "-Os")

include(common/FindVampirTrace.cmake)
if(VT_FOUND)
    add_library(bpVT SHARED ${PLUGIN_SOURCE})
    set_target_properties(bpVT PROPERTIES COMPILE_DEFINITIONS VT)
    foreach(dir ${VT_INCLUDE_DIRS})
        message(STATUS "including ${dir}")
    	include_directories(bpVT ${dir})
    endforeach(dir)
    target_link_libraries(bpVT pthread m)
endif()

include(common/FindScorep.cmake)
if(SCOREP_FOUND)
    add_library(bp SHARED ${PLUGIN_SOURCE})
    set_target_properties(bp PROPERTIES COMPILE_DEFINITIONS SCOREP)
    foreach(dir ${SCOREP_INCLUDE_DIRS})
        message(STATUS "including ${dir}")
    	include_directories(bp ${dir})
    endforeach(dir)
    target_link_libraries(bp bfd dl)
endif()

if(NOT SCOREP_FOUND AND NOT VT_FOUND)
    message(FATAL_ERROR "Found neither ScoreP or VampirTrace backend!")
endif()
