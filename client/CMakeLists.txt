cmake_minimum_required(VERSION 2.8)



# 设置头文件目录
project (client)

include_directories (inc)

aux_source_directory (src SOURCES)

add_executable (${PROJECT_NAME} ${SOURCES})

if (CMAKE_SYSTEM_NAME MATCHES "Darwin") # OSX 支持 libudev
  target_link_libraries (${PROJECT_NAME} osxfuse nanomsg pthread)
endif (CMAKE_SYSTEM_NAME MATCHES "Darwin")

if (CMAKE_SYSTEM_NAME MATCHES "Linux") # Linux 支持 libudev
  target_link_libraries (${PROJECT_NAME} fuse nanomsg pthread)
endif (CMAKE_SYSTEM_NAME MATCHES "Linux")

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
