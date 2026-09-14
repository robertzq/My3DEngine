#pragma once
#include "Engine/Behavior.h"

class CollectibleBehavior : public Behavior {
public:
    void OnSpawn(SceneContext& context) override;

    bool collected = false;
};
