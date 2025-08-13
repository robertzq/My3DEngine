cmake_minimum_required(VERSION 3.20)
project(My3DEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 开启多核编译更快
if(MSVC)
  add_compile_options(/MP)
endif()

# ===== 外部依赖 =====
include(FetchContent)

# glm
set(GLM_DIR ${CMAKE_SOURCE_DIR}/external/glm)
add_subdirectory(${GLM_DIR})

# glfw
set(GLFW_DIR ${CMAKE_SOURCE_DIR}/external/glfw)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
add_subdirectory(${GLFW_DIR})

# bx / bimg / bgfx




# 关闭示例、测试，只构建工具（会生成 shaderc）
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS     ON CACHE BOOL "" FORCE)

# （可选）新行为，避免 target_link_libraries 跨目录报错
if(POLICY CMP0079)
  cmake_policy(SET CMP0079 NEW)
endif()

# 本地引入
add_subdirectory(external/bgfx.cmake)
set(BGFX_DIR ${CMAKE_SOURCE_DIR}/external/bgfx.cmake)


# ===== 源码目标 =====
file(GLOB_RECURSE ENGINE_SRC
  src/main.cpp
  src/Engine/*.[ch]pp
  src/Engine/**/*.[ch]pp
  src/Engine/*.mm
  src/Engine/**/*.mm
  src/Game/*.[ch]pp
  src/Game/**/*.mm
)


add_executable(My3DEngine ${ENGINE_SRC})

# 用 FetchContent 的 include 路径，而不是 external/...
target_include_directories(My3DEngine PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bgfx/include
  ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bx/include
  ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bimg/include
)

target_link_libraries(My3DEngine PRIVATE
  bgfx
  bimg
  bx
  glfw
  glm
)
 target_compile_definitions(My3DEngine PRIVATE GLFW_INCLUDE_NONE)
set_target_properties(My3DEngine PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# macOS: 链接 Cocoa/QuartzCore/Metal
if(APPLE)

  find_library(COCOA_LIBRARY Cocoa)
  find_library(QUARTZCORE_LIBRARY QuartzCore)
  find_library(METAL_LIBRARY Metal)
  find_library(OPENGL_FRAMEWORK OpenGL)  
  target_link_libraries(My3DEngine PRIVATE ${COCOA_LIBRARY} ${QUARTZCORE_LIBRARY} ${METAL_LIBRARY} ${OPENGL_FRAMEWORK})
endif()



# 资源目录（运行时拷贝）
add_custom_target(copy_assets ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/bin/assets
)


set_target_properties(My3DEngine PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# ===== 自动编译 .sc -> .bin（输出到 shaders_bin） =====
set(SHADER_OUT_DIR ${CMAKE_BINARY_DIR}/bin/assets/shaders_bin)

add_custom_command(
  OUTPUT ${SHADER_OUT_DIR}/.dir_stamp
  COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUT_DIR}
  COMMAND ${CMAKE_COMMAND} -E touch ${SHADER_OUT_DIR}/.dir_stamp
)

file(GLOB VS_SHADERS ${CMAKE_SOURCE_DIR}/assets/shaders/vs_*.sc)
file(GLOB FS_SHADERS ${CMAKE_SOURCE_DIR}/assets/shaders/fs_*.sc)

set(SHADER_OUTPUTS "")

foreach(SRC ${VS_SHADERS})
  get_filename_component(FNAME ${SRC} NAME_WE)
  set(OUT_BIN ${SHADER_OUT_DIR}/${FNAME}.bin)
  add_custom_command(
    OUTPUT ${OUT_BIN}
    DEPENDS ${SRC} shaderc ${SHADER_OUT_DIR}/.dir_stamp
    COMMAND $<TARGET_FILE:shaderc>
      -f ${SRC} -o ${OUT_BIN}
      --type v --platform osx --profile metal
      --varying ${CMAKE_SOURCE_DIR}/assets/shaders/varying.def.sc
      -i ${BGFX_DIR}/bgfx/src
      -i ${BGFX_DIR}/bgfx/examples/common
      --verbose
    VERBATIM
  )
  list(APPEND SHADER_OUTPUTS ${OUT_BIN})
endforeach()

foreach(SRC ${FS_SHADERS})
  get_filename_component(FNAME ${SRC} NAME_WE)
  set(OUT_BIN ${SHADER_OUT_DIR}/${FNAME}.bin)
  add_custom_command(
    OUTPUT ${OUT_BIN}
    DEPENDS ${SRC} shaderc ${SHADER_OUT_DIR}/.dir_stamp
    COMMAND $<TARGET_FILE:shaderc>
      -f ${SRC} -o ${OUT_BIN}
      --type f --platform osx --profile metal
      --varying ${CMAKE_SOURCE_DIR}/assets/shaders/varying.def.sc
      -i ${BGFX_DIR}/bgfx/src
      -i ${BGFX_DIR}/bgfx/examples/common
      --verbose
    VERBATIM
  )
  list(APPEND SHADER_OUTPUTS ${OUT_BIN})
endforeach()

add_custom_target(compile_shaders ALL DEPENDS ${SHADER_OUTPUTS})

# 依赖顺序：先拷贝资源，再编 shader；最终 My3DEngine 依赖两者
add_dependencies(compile_shaders copy_assets)
add_dependencies(My3DEngine copy_assets)
add_dependencies(My3DEngine compile_shaders)

