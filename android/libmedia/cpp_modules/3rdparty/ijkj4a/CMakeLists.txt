cmake_minimum_required(VERSION 3.4.1)
set(SUFFIX  *.c *.h)
set(MODULE ijkj4a)

file(GLOB_RECURSE SRCS ${SUFFIX})


add_library(${MODULE} STATIC ${SRCS})

# Specify directories which the compiler should look for headers
target_include_directories(${MODULE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

#target_compile_options(${MODULE} PRIVATE -std=c++11
#                            PRIVATE -Wall
#                            PRIVATE "$<$<CONFIG:DEBUG>:-Werror>") # Only include -Werror when building debug config

target_link_libraries(${MODULE} log)