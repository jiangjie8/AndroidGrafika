cmake_minimum_required (VERSION 2.8)
cmake_policy(VERSION 2.8)

project (av_process)

add_definitions(-DWin)

if (WIN32)
    set(SYSTEM_PLATFORM "Windows")
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
endif(WIN32)

set(3RDPART_PATH "${CMAKE_SOURCE_DIR}/../../3rdparty/win")
set(CORE_PATH "${CMAKE_SOURCE_DIR}/../../source")

set(FFMPEG_PATH "${3RDPART_PATH}/ffmpeg")
include_directories(${FFMPEG_PATH}/include)
link_directories(${FFMPEG_PATH}/lib )
set(FFMPEG_LIBRARYS avcodec avformat avutil avdevice avfilter postproc swresample swscale)

set(CORE_MODULE libcore)
file(GLOB CORE_SRCS
    ${CORE_PATH}/core/*.h
    ${CORE_PATH}/core/*.cpp
    ${CORE_PATH}/core/media_common/*.h
    ${CORE_PATH}/core/media_common/*.cpp
    ${CORE_PATH}/core/utils/*.h
    ${CORE_PATH}/core/utils/*.cpp
    )
add_library(${CORE_MODULE} STATIC ${CORE_SRCS})
target_include_directories(${CORE_MODULE} PRIVATE ${CORE_PATH})

add_compile_options(-std=c++11)
add_definitions(-D__STDC_CONSTANT_MACROS)  

set(PROJECTS
    concat_file
)


foreach(PROJECT  ${PROJECTS})
    add_subdirectory(source/${PROJECT})
endforeach(PROJECT)

#MESSAGE(">>>" ${INCLUDE_DIRECTORIES})



