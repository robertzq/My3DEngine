#pragma once
#include "Engine/SceneController.h"

class PlayController : public SceneController {
public:
    void OnEnter(SceneContext& context) override;
    void Update(SceneContext& context) override;
};
