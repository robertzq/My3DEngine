#pragma once

// 引擎全局常量：集中管理，避免散落各处的魔法数字
namespace EngineConfig {
    constexpr int TILE_SIZE = 32;         // 单个图块像素尺寸
    constexpr int SCREEN_WIDTH = 800;     // 窗口宽度
    constexpr int SCREEN_HEIGHT = 600;    // 窗口高度
    constexpr int TARGET_FPS = 60;        // 目标帧率
}
