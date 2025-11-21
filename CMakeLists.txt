cmake_minimum_required(VERSION 3.14)
project(MyEngine)

set(CMAKE_CXX_STANDARD 17)

# --- 0. 通用配置：头文件搜索路径 ---
# 告诉编译器去 src 目录找 Game.h, Map.h 等
include_directories(src)

# --- 1. 平台判定逻辑 ---
if(WIN32)
    # ================= Windows 配置 =================
    message(STATUS "Detecting Windows system...")

    # 设定依赖库的路径 (请在 Windows 上把库解压到项目根目录的 dependencies 文件夹)
    set(SDL2_DIR "${CMAKE_SOURCE_DIR}/dependencies/SDL2")
    set(SDL2_IMAGE_DIR "${CMAKE_SOURCE_DIR}/dependencies/SDL2_image")

    # 添加头文件路径
    include_directories(${SDL2_DIR}/include)
    include_directories(${SDL2_IMAGE_DIR}/include)

    # 寻找 .lib 文件 (假设是 64位 x64)
    # 注意：Windows 库通常在 lib/x64 下
    find_library(SDL2_LIB NAMES SDL2 PATHS ${SDL2_DIR}/lib/x64)
    find_library(SDL2_MAIN_LIB NAMES SDL2main PATHS ${SDL2_DIR}/lib/x64)
    find_library(SDL2_IMAGE_LIB NAMES SDL2_image PATHS ${SDL2_IMAGE_DIR}/lib/x64)

elseif(APPLE)
    # ================= macOS 配置 (保留你原来的逻辑) =================
    message(STATUS "Detecting macOS system...")

    # 自动判断是 Apple Silicon 还是 Intel
    if(EXISTS "/opt/homebrew/include")
        set(HOMEBREW_PREFIX "/opt/homebrew")
    else()
        set(HOMEBREW_PREFIX "/usr/local")
    endif()

    # 包含路径
    include_directories(${HOMEBREW_PREFIX}/include)
    include_directories(${HOMEBREW_PREFIX}/include/SDL2)

    # 库路径
    link_directories(${HOMEBREW_PREFIX}/lib)

else()
    # ================= Linux (预留) =================
    find_package(SDL2 REQUIRED)
    find_package(SDL2_image REQUIRED)
endif()

# --- 2. 源文件列表 ---
set(SOURCES
    src/main.cpp
    src/Game.cpp
    src/TextureManager.cpp
    src/GameObject.cpp
    src/Map.cpp
    src/Collision.cpp
    src/GiftBox.cpp      # 注意：请确认文件名大小写！是 GiftBox.cpp 还是 Giftbox.cpp
    src/Collectible.cpp
)

# --- 3. 创建可执行文件 ---
add_executable(MyEngine ${SOURCES})

# --- 4. 链接库 (根据平台区分) ---
if(WIN32)
    # Windows 链接逻辑
    if(NOT SDL2_LIB OR NOT SDL2_IMAGE_LIB)
        message(FATAL_ERROR "Windows libraries not found! Please check dependencies folder.")
    endif()
    target_link_libraries(MyEngine ${SDL2_MAIN_LIB} ${SDL2_LIB} ${SDL2_IMAGE_LIB})
elseif(APPLE)
    # Mac 链接逻辑 (保持你原来的写法)
    target_link_libraries(MyEngine SDL2 SDL2_image)
else()
    target_link_libraries(MyEngine SDL2::SDL2 SDL2_image::SDL2_image)
endif()