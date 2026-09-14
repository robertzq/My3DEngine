#pragma once
#include "Engine/SceneView.h"
#include "Controllers/VillageController.h"

class VillageView : public SceneView {
public:
    void OnEnter(SceneContext& context) override;
    void Render(SceneContext& context) override;

private:
    void DrawCutscene(SceneContext& context);
    void DrawWordCloud(SceneContext& context);
    void DrawText(const std::string& text, int x, int y, SDL_Color color);

    VillageController* controller = nullptr;
};
