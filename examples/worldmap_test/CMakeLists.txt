# WorldMap / TileLayer 多层地图单元测试（阶段3）。
# 独立 target，由引擎根 MYENGINE_BUILD_TESTS=ON 时启用（默认 OFF，不影响游戏库）。
# 本测试是纯逻辑 console，不调用 myengine_setup_target（避免 SDL2main 劫持 main）。

add_executable(WorldMapTest
    main.cpp
)

target_include_directories(WorldMapTest PRIVATE ${CMAKE_SOURCE_DIR}/engine/include)
target_link_libraries(WorldMapTest PRIVATE engine)