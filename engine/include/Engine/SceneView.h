#pragma once
#include <SDL.h>
#include "Engine/SceneContext.h"

class SceneView {
public:
    virtual ~SceneView() = default;

    virtual void OnEnter(SceneContext& context) {}
    virtual void OnExit() {}
    virtual void Render(SceneContext& context) = 0;
};
