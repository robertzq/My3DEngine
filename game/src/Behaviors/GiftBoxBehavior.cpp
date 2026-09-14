#include "Behaviors/GiftBoxBehavior.h"
#include "Engine/BehaviorRegistry.h"
#include "Engine/Entity.h"
#include "Engine/ResourceManager.h"

void GiftBoxBehavior::OnSpawn(SceneContext& context) {
    self->sprite.texture = ResourceManager::GetTexture("gift.png");
    bannerTexture = ResourceManager::GetTexture("birthbanner.png");

    if (self->transform.w == 0) self->transform.w = 64;
    if (self->transform.h == 0) self->transform.h = 64;

    self->collider.offset = {0, 0, self->transform.w, self->transform.h};
    self->collider.enabled = true;
}

void GiftBoxBehavior::Open() {
    if (opened) return;
    opened = true;
    openTime = SDL_GetTicks();
}

static BehaviorRegistry::Proxy proxy_gift_box("GiftBox", []() {
    return new GiftBoxBehavior();
});
