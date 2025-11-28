#pragma once
#include "Scene.h"
#include "GameObject.h"
#include "Map.h"
#include "GiftBox.h"
#include "Collectible.h"
#include <vector>
#include <string>

class PlayScene : public Scene {
public:
    PlayScene(std::string mapFile);

    void OnEnter() override;
    void OnExit() override;
    void HandleEvents(SDL_Event& event) override;
    void Update() override;
    void Render() override;

private:
    std::string mapPath;

    GameObject* player = nullptr;
    Map* map = nullptr;
    GiftBox* giftBox = nullptr;
    std::vector<Collectible*> hearts;
};