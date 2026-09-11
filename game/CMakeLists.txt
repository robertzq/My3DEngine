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

add_executable(MyEngine
    src/main.cpp
    src/RPGPlayer.cpp
    src/GiftBox.cpp
    src/Collectible.cpp
    src/Map.cpp
    src/Scenes/VillageScene.cpp
    src/Scenes/BattleScene.cpp
    src/Scenes/PlayScene.cpp
)

target_include_directories(MyEngine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(MyEngine PRIVATE engine)

if(TARGET embed_assets)
    add_dependencies(MyEngine embed_assets)
endif()

# 输出到 build/ 根目录，保持 runGame.sh 路径不变
set_target_properties(MyEngine PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
