cmake_minimum_required(VERSION 3.14)
project(MyEngine)

set(CMAKE_CXX_STANDARD 17)

# ==========================================
# 【核心修复】强制 Windows 使用 UTF-8 编码
# 这一步能消除 C4819 警告，并修复 C2001 错误
# ==========================================
if(MSVC)
    add_compile_options("/utf-8")
endif()
# ==========================================
# =============================================================
#  1. 智能源码扫描 (类似 Spring 的 Component Scan)
# =============================================================
# file(GLOB_RECURSE ...) 会递归查找 src 目录下所有的 .cpp 文件
# 将结果存入 SOURCES 变量中
file(GLOB_RECURSE SOURCES "src/*.cpp")

# 打印一下找到的文件，方便调试（编译时会在控制台看到）
message(STATUS "自动扫描到的源文件:")
foreach(f ${SOURCES})
    message(STATUS "  -> ${f}")
endforeach()

# =============================================================
#  2. 智能头文件路径处理 (支持分包)
# =============================================================
# 基础：包含 src 根目录
include_directories(src)

# 进阶：自动扫描 src 下所有的子文件夹，并加入 include 路径
# 这样你可以随意分包，引用时依然可以直接写文件名，或者 "包名/文件名"
file(GLOB_RECURSE ALL_HEADERS "src/*.h")
set(ALL_SUBDIRS "")
foreach(HEADER ${ALL_HEADERS})
    get_filename_component(DIR ${HEADER} DIRECTORY)
    list(APPEND ALL_SUBDIRS ${DIR})
endforeach()
# 去重，防止同一个目录被添加多次
list(REMOVE_DUPLICATES ALL_SUBDIRS)

# 将所有子目录加入头文件搜索路径
foreach(SUBDIR ${ALL_SUBDIRS})
    include_directories(${SUBDIR})
endforeach()

# =============================================================
#  3. 平台依赖配置 (保持不变)
# =============================================================
if(WIN32)
    message(STATUS "Detecting Windows system...")
    set(SDL2_DIR "${CMAKE_SOURCE_DIR}/dependencies/SDL2")
    set(SDL2_IMAGE_DIR "${CMAKE_SOURCE_DIR}/dependencies/SDL2_image")
    set(SDL2_TTF_DIR "${CMAKE_SOURCE_DIR}/dependencies/SDL2_ttf")
    include_directories(${SDL2_DIR}/include)
    include_directories(${SDL2_IMAGE_DIR}/include)
    include_directories(${SDL2_TTF_DIR}/include)
    find_library(SDL2_LIB NAMES SDL2 PATHS ${SDL2_DIR}/lib/x64)
    find_library(SDL2_MAIN_LIB NAMES SDL2main PATHS ${SDL2_DIR}/lib/x64)
    find_library(SDL2_IMAGE_LIB NAMES SDL2_image PATHS ${SDL2_IMAGE_DIR}/lib/x64)
    find_library(SDL2_TTF_LIB NAMES SDL2_ttf PATHS ${SDL2_TTF_DIR}/lib/x64)
elseif(APPLE)
    message(STATUS "Detecting macOS system...")
    if(EXISTS "/opt/homebrew/include")
        set(HOMEBREW_PREFIX "/opt/homebrew")
    else()
        set(HOMEBREW_PREFIX "/usr/local")
    endif()
    include_directories(${HOMEBREW_PREFIX}/include)
    include_directories(${HOMEBREW_PREFIX}/include/SDL2)
    link_directories(${HOMEBREW_PREFIX}/lib)
else()
    find_package(SDL2 REQUIRED)
    find_package(SDL2_image REQUIRED)
    find_package(SDL2_ttf REQUIRED)
endif()

# =============================================================
#  4. 构建目标
# =============================================================
add_executable(MyEngine ${SOURCES})

if(WIN32)
    if(NOT SDL2_LIB OR NOT SDL2_IMAGE_LIB OR NOT SDL2_TTF_LIB)
        message(FATAL_ERROR "Windows libraries not found!")
    endif()
    target_link_libraries(MyEngine ${SDL2_MAIN_LIB} ${SDL2_LIB} ${SDL2_IMAGE_LIB} ${SDL2_TTF_LIB})
elseif(APPLE)
    target_link_libraries(MyEngine SDL2 SDL2_image SDL2_ttf)
else()
    target_link_libraries(MyEngine SDL2::SDL2 SDL2_image::SDL2_image SDL2_ttf::SDL2_ttf)
endif()