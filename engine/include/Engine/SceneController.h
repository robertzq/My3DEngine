#pragma once
#include <SDL.h>
#include "Engine/SceneContext.h"

class SceneController {
public:
    virtual ~SceneController() = default;

    virtual void OnEnter(SceneContext& context) {}
    virtual void OnExit() {}
    virtual void HandleEvent(SceneContext& context, SDL_Event& event) {}
    virtual void Update(SceneContext& context) = 0;
};
