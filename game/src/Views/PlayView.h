#pragma once
#include "Engine/SceneView.h"
#include "Controllers/PlayController.h"

class PlayView : public SceneView {
public:
    void OnEnter(SceneContext& context) override;
    void Render(SceneContext& context) override;

private:
    PlayController* controller = nullptr;
};
