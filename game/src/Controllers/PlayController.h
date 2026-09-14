#pragma once
#include <vector>
#include "Engine/SceneController.h"
#include "Collectible.h"
#include "Engine/GameObject.h"
#include "GiftBox.h"

class PlayController : public SceneController {
public:
    void OnEnter(SceneContext& context) override;
    void OnExit() override;
    void HandleEvent(SceneContext& context, SDL_Event& event) override;
    void Update(SceneContext& context) override;

    GameObject* player = nullptr;
    GiftBox* giftBox = nullptr;
    std::vector<Collectible*> hearts;
};
