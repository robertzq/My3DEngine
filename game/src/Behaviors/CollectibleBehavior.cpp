#include "Behaviors/CollectibleBehavior.h"
#include "Engine/BehaviorRegistry.h"
#include "Engine/Entity.h"
#include "Engine/ResourceManager.h"

void CollectibleBehavior::OnSpawn(SceneContext& context) {
    self->sprite.texture = ResourceManager::GetTexture("heart.png");

    if (self->transform.w == 0) self->transform.w = 40;
    if (self->transform.h == 0) self->transform.h = 32;

    self->collider.offset = {0, 0, self->transform.w, self->transform.h};
    self->collider.enabled = true;
}

static BehaviorRegistry::Proxy proxy_collectible("Collectible", []() {
    return new CollectibleBehavior();
});
