#pragma once
#include <SDL.h>
#include "Engine/Behavior.h"

class GiftBoxBehavior : public Behavior {
public:
    void OnSpawn(SceneContext& context) override;

    void Open();
    bool IsOpened() const { return opened; }
    void CloseBanner() { bannerDismissed = true; }
    bool IsBannerClosed() const { return bannerDismissed; }

    SDL_Texture* bannerTexture = nullptr;
    Uint32 openTime = 0;

private:
    bool opened = false;
    bool bannerDismissed = false;
};
