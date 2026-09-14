#pragma once
#include <SDL.h>
#include <string>
#include <vector>
#include "Engine/SceneController.h"
#include "GiftBox.h"
#include "RPGPlayer.h"

enum class VillageState {
    Playing,
    Scanning,
    WordCloud
};

struct CloudTag {
    std::string text;
    int targetX = 0;
    int targetY = 0;
    float currentX = 0.0f;
    float currentY = 0.0f;
    int fontSize = 16;
    SDL_Color color = {255, 255, 255, 255};
};

class VillageController : public SceneController {
public:
    void OnEnter(SceneContext& context) override;
    void OnExit() override;
    void HandleEvent(SceneContext& context, SDL_Event& event) override;
    void Update(SceneContext& context) override;

    RPGPlayer* player = nullptr;
    std::vector<GiftBox*> gifts;

    VillageState state = VillageState::Playing;
    std::vector<std::string> logLines;
    std::vector<CloudTag> tags;
    bool cloudFormed = false;
    Uint32 stateStartTime = 0;

private:
    void StartScan(SceneContext& context);
    void FinishScan(SceneContext& context);
    void BuildWordCloud();

    std::string mapId;
    int scanStage = 0;
};
