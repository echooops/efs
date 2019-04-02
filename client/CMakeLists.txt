cmake_minimum_required(VERSION 2.8)



# 设置头文件目录
project (client)

include_directories (inc)

aux_source_directory (src SOURCES)

add_executable (${PROJECT_NAME} ${SOURCES})

target_link_libraries (${PROJECT_NAME} osxfuse nanomsg pthread)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
