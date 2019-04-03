cmake_minimum_required (VERSION 2.8)

# 个人需求 -- 辛爱国
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

message (STATUS "operation system : ${CMAKE_SYSTEM}")
message (STATUS "system name      : ${CMAKE_SYSTEM_NAME}")

# 设置工程常用路径
set (ROOT_DIR ${PROJECT_SOURCE_DIR})
message (STATUS "root dir         : ${ROOT_DIR}")
# 工具库
set (LIBS_DIR ${ROOT_DIR}/libs)
message (STATUS "libs dir         : ${LIBS_DIR}")
# 工具库
set (UTILS_DIR ${ROOT_DIR}/utils)
message (STATUS "utils dir        : ${UTILS_DIR}")


include_directories (${ROOT_DIR})
include_directories (${UTILS_DIR})
# 通信需要包含
include_directories (${LIBS_DIR}/nanomsg-1.1.5/src/)
# 日志需要包含
include_directories (${LIBS_DIR}/spdlog-1.3.1/include/)
# osxfuse 头文件
if (CMAKE_SYSTEM_NAME MATCHES "Darwin") # Linux 支持 libudev
  include_directories (/usr/local/include/osxfuse)
endif (CMAKE_SYSTEM_NAME MATCHES "Darwin")

link_directories (/usr/local/lib)

# DEBUG 模式
add_definitions (-DDEBUG)


add_subdirectory (client)
add_subdirectory (server)

# 编译一来库
add_subdirectory (${LIBS_DIR}/nanomsg-1.1.5)


# 配置 CMAKE 编译参数
set (CMAKE_C_COMPILER g++)
set (CMAKE_CXX_COMPILER g++)

# 使用 c++11
include (CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if (COMPILER_SUPPORTS_CXX11)
  message(STATUS "compiler supports c++11")
  if (CMAKE_COMPILER_IS_GUNCXX)
    add_compile_options(-std=c++11)
  else (CMAKE_COMPILER_IS_GUNCXX)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  endif (CMAKE_COMPILER_IS_GUNCXX)
else (COMPILER_SUPPORTS_CXX11)
  message(FATAL_ERROR "compiler not supports c++11")
endif (COMPILER_SUPPORTS_CXX11)
