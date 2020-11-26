cmake_minimum_required(VERSION 2.8)

project(jagunmolu-player)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -Wall -Wextra")

include_directories(include)

list(APPEND SOURCES
        src/FFMpegIOContext.cpp
        src/FFMpegMediaPlayer.cpp
        src/FFMpegDecoder.cpp
        src/FFMpegDemuxer.cpp
        src/FFMpegMedia.cpp
        src/SDLAudioOutput.cpp
        src/Timer.cpp
		src/FFMpegResampler.cpp
		src/FFMpegFilter.cpp
		src/FFMpegFilterGraph.cpp)

add_library(${PROJECT_NAME} ${SOURCES})

add_executable(jagunmolu-player-test test/main.cpp)

target_link_libraries(${PROJECT_NAME} avutil avformat avcodec swresample avfilter SDL2 pthread)

target_link_libraries(jagunmolu-player-test ${PROJECT_NAME})
