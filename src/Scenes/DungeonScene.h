// src/Scenes/DungeonScene.h
#pragma once
#include "Scene.h"
#include "GameObject.h"
#include "Map.h"
#include <string>
#include <vector>

class DungeonScene : public Scene {
public:
    // 增加一个可选参数：spawnAtEntrance，用于指示是否需要出生在特定点
    DungeonScene(std::string mapFile, bool spawnAtEntrance = false);

    void OnEnter() override;
    void OnExit() override;
    void HandleEvents(SDL_Event& event) override;
    void Update() override;
    void Render() override;

private:
    std::string mapPath;
    bool shouldSpawnAtEntrance; // 是否需要寻找出生点

    GameObject* player;
    Map* map;

    // 触发器列表
    std::vector<SDL_Rect> doorTriggers; // ID: 7 (进房子)
    std::vector<SDL_Rect> bossTriggers; // ID: 14 (打BOSS)
};