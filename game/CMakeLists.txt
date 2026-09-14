# ============ 游戏（Memory Island demo） ============
# 依赖引擎库 engine，只包含游戏自身的资源与逻辑。

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
    src/Behaviors/PlayerBehavior.cpp
    src/Views/WorldView.cpp
)

target_include_directories(MyEngine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(MyEngine PRIVATE engine)

if(TARGET embed_assets)
    add_dependencies(MyEngine embed_assets)
endif()

set_target_properties(MyEngine PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
