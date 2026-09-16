#pragma once
#include "Engine/Projection.h"
#include "Engine/SceneData.h"
#include "Engine/WorldMap.h"

class Game;
class SceneManager;
class SceneController;

struct SceneContext {
    Game* game = nullptr;
    SceneManager* manager = nullptr;
    SceneController* controller = nullptr;
    WorldMap* map = nullptr;
    const SceneData* data = nullptr;
    json params;

    // 当前场景的投影配置（正交平铺 / 斜等距）。
    // SceneManager::LoadScene 时按 SceneData.projectionMode 填充。
    Projection projection;
};