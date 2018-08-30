# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.


set(MODULE avmedia)

file(GLOB SRCS
    ${CMAKE_SOURCE_DIR}/*.h
    ${CMAKE_SOURCE_DIR}/*.cpp
    ${CMAKE_SOURCE_DIR}/media_common/*.h
    ${CMAKE_SOURCE_DIR}/media_common/*.cpp
    )
add_library(${MODULE} SHARED ${SRCS})
target_include_directories(${MODULE} PRIVATE ${CMAKE_SOURCE_DIR})
#target_include_directories(${MODULE} PRIVATE ${CMAKE_SOURCE_DIR}/media_common)


#c++ wrapper java class
set(IJKJ4A_PATH ${CMAKE_SOURCE_DIR}/3rdparty/ijkj4a)
add_subdirectory(${IJKJ4A_PATH})
target_link_libraries(${MODULE} ijkj4a)
target_include_directories(${MODULE} PRIVATE ${IJKJ4A_PATH})


#liboboe
set(OBOE_PATH ${CMAKE_SOURCE_DIR}/3rdparty/oboe)
add_subdirectory(${OBOE_PATH})
target_link_libraries(${MODULE} oboe)
target_include_directories(${MODULE} PRIVATE ${OBOE_PATH}/include)



set(3RDPARTY_PATH ${CMAKE_SOURCE_DIR}/3rdparty)

#libffmpeg
set(FFMPEG_PATH ${3RDPARTY_PATH}/ffmpeg/${ANDROID_ABI})
target_include_directories(${MODULE} PRIVATE ${FFMPEG_PATH}/include)
target_link_libraries(${MODULE} ${CMAKE_SOURCE_DIR}/../libs/${ANDROID_ABI}/libffmpeg.so)
add_definitions(-D__STDC_CONSTANT_MACROS)


find_library(LOG_LIB log)
target_link_libraries(${MODULE} ${LOG_LIB})

#link_directories(${FFMPEG_PATH})
#find_library(FFMPEG_LIB
#                NAMES ffmpeg
#                HINTS "${FFMPEG_PATH}"
#                PATHS "${FFMPEG_PATH}"
#                NO_DEFAULT_PATH)
#MESSAGE(">>>>"  "${FFMPEG_PATH}, ${CMAKE_SOURCE_DIR}/../../../libs/${ANDROID_ABI}")