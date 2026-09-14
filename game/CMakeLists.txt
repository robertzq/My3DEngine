# ============ 游戏（生日 RPG demo） ============
# 依赖引擎库 engine，只包含游戏自身的资源与逻辑。

# --- 自动生成嵌入式资源 (EmbeddedAssets.h) ---
# embed_assets.py 内部做了增量判断：资源未变化时跳过，
# 避免每次构建都重写大文件导致全量重编译
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
    add_custom_target(embed_assets ALL
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_assets.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Embedding assets into EmbeddedAssets.h"
        VERBATIM
    )
else()
    message(WARNING "未找到 Python3，请手动运行: python game/tools/embed_assets.py")
endif()

add_executable(MyEngine WIN32
    src/main.cpp
    src/Behaviors/PlayerBehavior.cpp
    src/Behaviors/GiftBoxBehavior.cpp
    src/Behaviors/CollectibleBehavior.cpp
    src/Controllers/VillageController.cpp
    src/Controllers/BattleController.cpp
    src/Controllers/PlayController.cpp
    src/Views/VillageView.cpp
    src/Views/BattleView.cpp
    src/Views/PlayView.cpp
    src/Views/GiftBanner.cpp
)

target_include_directories(MyEngine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(MyEngine PRIVATE engine)

if(TARGET embed_assets)
    add_dependencies(MyEngine embed_assets)
endif()

# 输出到 build/ 根目录，保持 runGame.sh 路径不变
set_target_properties(MyEngine PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Auto-copy SDL2 runtime DLLs to the executable output directory (Windows)
if(WIN32)
    add_custom_command(TARGET MyEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/dependencies/SDL2/lib/x64/SDL2.dll"
            "${CMAKE_SOURCE_DIR}/dependencies/SDL2_image/lib/x64/SDL2_image.dll"
            "${CMAKE_SOURCE_DIR}/dependencies/SDL2_ttf/lib/x64/SDL2_ttf.dll"
            "${CMAKE_SOURCE_DIR}/dependencies/SDL2_mixer/lib/x64/SDL2_mixer.dll"
            $<TARGET_FILE_DIR:MyEngine>
        COMMENT "Copying SDL2 runtime DLLs to output directory"
        VERBATIM
    )
endif()