#pragma once
#include "Engine/SceneView.h"

class WorldView : public SceneView {
public:
    void Render(SceneContext& context) override;
};
