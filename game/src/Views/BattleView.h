#pragma once
#include <SDL.h>
#include <string>
#include "Engine/SceneView.h"
#include "Controllers/BattleController.h"

class BattleView : public SceneView {
public:
    void OnEnter(SceneContext& context) override;
    void Render(SceneContext& context) override;

private:
    void DrawHPBar(int x, int y, int current, int max, SDL_Color color);

    BattleController* controller = nullptr;
};
