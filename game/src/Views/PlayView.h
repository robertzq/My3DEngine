#pragma once
#include "Engine/SceneView.h"

class PlayView : public SceneView {
public:
    void Render(SceneContext& context) override;
};
