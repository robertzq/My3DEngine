#pragma once
#include "Engine/SceneData.h"
#include "Engine/TileMap.h"

class Game;
class SceneManager;
class SceneController;

struct SceneContext {
    Game* game = nullptr;
    SceneManager* manager = nullptr;
    SceneController* controller = nullptr;
    TileMap* map = nullptr;
    const SceneData* data = nullptr;
    json params;
};
