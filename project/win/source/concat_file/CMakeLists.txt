set(MODULE concat_file)

set(SUFFIX  *.cpp *.h)
file(GLOB SRCS
    ${SUFFIX})

add_executable(${MODULE} ${SRCS})
target_include_directories(${MODULE} PRIVATE ${CORE_PATH})
target_link_libraries(${MODULE} ${FFMPEG_LIBRARYS} )
target_link_libraries(${MODULE} ${CORE_MODULE})

add_dependencies(${MODULE} ${CORE_MODULE})
