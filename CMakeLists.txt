cmake_minimum_required(VERSION 3.14)
project(MyEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 【核心修复】强制 Windows 使用 UTF-8 编码
if(MSVC)
    add_compile_options("/utf-8")
endif()

# 引擎库
add_subdirectory(engine)

# ??????? -DMYENGINE_BUILD_DEMO=OFF ???
option(MYENGINE_BUILD_DEMO "Build the MyEngine demo executable" ON)
if(MYENGINE_BUILD_DEMO)
    add_subdirectory(examples/demo)
endif()

# WorldMap/TileLayer 多层地图单元测试（默认 OFF，不影响游戏库构建）
option(MYENGINE_BUILD_TESTS "Build engine unit tests" OFF)
if(MYENGINE_BUILD_TESTS)
    add_subdirectory(examples/worldmap_test)
endif()
