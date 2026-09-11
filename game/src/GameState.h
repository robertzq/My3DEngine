#pragma once

// 游戏全局状态（引擎无关的生日 RPG 状态）
// 引擎 Game 只负责循环/场景，游戏自身的进度状态放这里。
struct GameState {
    int bossDefeatedCount = 0; // 打败 BOSS 数量
    int lastVillageX = -1;     // 回村庄时的记忆坐标
    int lastVillageY = -1;
};

// 全局单例（C++17 inline 变量，跨编译单元唯一）
inline GameState g_gameState;
