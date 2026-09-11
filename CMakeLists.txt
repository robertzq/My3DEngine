cmake_minimum_required(VERSION 3.14)
project(MyGameEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 【核心修复】强制 Windows 使用 UTF-8 编码
if(MSVC)
    add_compile_options("/utf-8")
endif()

# 引擎库 + 游戏项目
add_subdirectory(engine)
add_subdirectory(game)
