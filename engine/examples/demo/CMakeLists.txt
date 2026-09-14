# MyEngine 最小演示程序
# 演示如何用引擎构建一个可执行目标，并复用 myengine_setup_target() 的
# 依赖查找 / GUI 子系统 / DLL 自动部署能力。
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/engine/cmake" ${CMAKE_MODULE_PATH})
include(EngineDeps)

add_executable(MyEngineDemo WIN32
    main.cpp
)

target_link_libraries(MyEngineDemo PRIVATE engine)

# 一键装配：SDL 查找校验 + GUI 子系统 + DLL 自动部署
myengine_setup_target(MyEngineDemo)