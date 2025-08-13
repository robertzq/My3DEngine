cmake_minimum_required(VERSION 3.14)
project(MyEngine)

set(CMAKE_CXX_STANDARD 17)

# --- 1. 定义 Homebrew 的路径 ---
# 自动判断是 Apple Silicon (/opt/homebrew) 还是 Intel (/usr/local)
if(EXISTS "/opt/homebrew/include")
    set(HOMEBREW_PREFIX "/opt/homebrew")
else()
    set(HOMEBREW_PREFIX "/usr/local")
endif()

# --- 2. 显式包含路径 ---
# 告诉 CMake 去这两个地方找头文件
include_directories(${HOMEBREW_PREFIX}/include)
include_directories(${HOMEBREW_PREFIX}/include/SDL2)

# 告诉 CMake 去这个地方找库文件 (.dylib)
link_directories(${HOMEBREW_PREFIX}/lib)

# --- 【关键修改 1】添加头文件搜索路径 ---
# 告诉编译器：如果在代码里写 #include "Game.h"，请去 src 目录里找
include_directories(src)

# --- 【关键修改 2】更新源文件路径 ---
# 所有的文件名前面都要加上 src/
set(SOURCES
    src/main.cpp
    src/Game.cpp
    src/TextureManager.cpp
    src/GameObject.cpp
)

# 创建可执行文件
add_executable(MyEngine ${SOURCES})

# --- 4. 链接库 ---
# 直接链接 SDL2 和 SDL2_image
target_link_libraries(MyEngine SDL2 SDL2_image)