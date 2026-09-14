# ============ MyEngine 依赖装配辅助函数 ============
# 封装「用本引擎做游戏」时的通用构建能力：
#   1. 查找 SDL2 系列运行时依赖（SDL2 / SDL2_image / SDL2_ttf / SDL2_mixer）
#   2. 配置 Windows GUI 子系统（游戏默认不弹控制台黑框）
#   3. 将 SDL 运行库 DLL 自动复制到目标可执行文件输出目录
#
# 使用方式（在游戏子 CMakeLists 中）：
#   set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/engine/cmake" ${CMAKE_MODULE_PATH})
#   include(EngineDeps)
#   add_executable(MyGame WIN32 ...)
#   target_link_libraries(MyGame PRIVATE engine)
#   myengine_setup_target(MyGame)
#
# 注：依赖目录默认放在 ${CMAKE_SOURCE_DIR}/dependencies 下，
#   结构与 engine/CMakeLists.txt 的 WIN32 分支保持一致。

# ---------------------------------------------------------------------------
function(myengine_setup_target TARGET)
    # --- 1. 平台依赖查找（仅 Windows 需要 DLL 部署；库链接由 engine 传递） ---
    if(WIN32)
        message(STATUS "myengine: locating SDL2 runtime libraries...")
        set(_DEPS "${CMAKE_SOURCE_DIR}/dependencies")

        set(_REQUIRED_DLLS
            "${_DEPS}/SDL2/lib/x64/SDL2.dll"
            "${_DEPS}/SDL2_image/lib/x64/SDL2_image.dll"
            "${_DEPS}/SDL2_ttf/lib/x64/SDL2_ttf.dll"
            "${_DEPS}/SDL2_mixer/lib/x64/SDL2_mixer.dll"
        )

        # 校验依赖是否齐备
        foreach(_dll IN LISTS _REQUIRED_DLLS)
            if(NOT EXISTS "${_dll}")
                message(FATAL_ERROR "myengine: 缺少 SDL 依赖: ${_dll}。请参考 engine/CMakeLists.txt 下载对应开发包到 dependencies/ 目录。")
            endif()
        endforeach()

        # --- 2. GUI 子系统：链接 SDL2main（提供 WinMain）并移除控制台黑框 ---
        # add_executable 需声明 WIN32；此处额外把 SDL2main 前置，保证入口正确
        set_property(TARGET ${TARGET} PROPERTY WIN32_EXECUTABLE TRUE)

        # --- 3. POST_BUILD 自动复制 DLL 到可执行文件输出目录 ---
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${_REQUIRED_DLLS}
                $<TARGET_FILE_DIR:${TARGET}>
            COMMENT "myengine: copying SDL2 runtime DLLs to $<TARGET_FILE_DIR:${TARGET}>"
            VERBATIM
        )
    endif()
endfunction()