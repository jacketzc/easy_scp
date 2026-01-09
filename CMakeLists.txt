cmake_minimum_required(VERSION 3.10)
project(easy_scp C)

set(CMAKE_C_STANDARD 11)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIRS})  # 注意：有些 CMake 版本用 _DIRS

add_executable(easy_scp main.c
        config.c
        config.h)
target_link_libraries(easy_scp ${CURSES_LIBRARIES})
