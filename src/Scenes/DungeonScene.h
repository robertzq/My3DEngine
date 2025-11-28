#pragma once
#include "Scene.h"
#include "GameObject.h"
#include "Map.h"
#include <string>

class DungeonScene : public Scene {
public:
    DungeonScene(std::string mapFile); // 构造函数

    void OnEnter() override;
    void OnExit() override;
    void HandleEvents(SDL_Event& event) override;
    void Update() override;
    void Render() override;

private:
    std::string mapPath;
    GameObject* player;
    Map* map;

    // 【新增】存储触发器区域
    std::vector<SDL_Rect> triggers;
};