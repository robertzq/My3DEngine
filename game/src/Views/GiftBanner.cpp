#include "Views/GiftBanner.h"
#include "Behaviors/GiftBoxBehavior.h"
#include "Engine/Entity.h"
#include "Engine/Game.h"
#include "Engine/Renderer.h"
#include "Engine/SceneManager.h"
#include "Engine/TextRenderer.h"

void DrawGiftBanners(SceneContext& context) {
    if (!context.manager) return;

    for (Entity* entity : context.manager->Entities()) {
        auto* gift = dynamic_cast<GiftBoxBehavior*>(entity->behavior.get());
        if (!gift || !gift->IsOpened() || gift->IsBannerClosed() || !gift->bannerTexture) continue;

        SDL_Rect banner;
        banner.x = static_cast<int>(entity->transform.x) + entity->transform.w / 2 - 200 - Game::camera.x;
        banner.y = static_cast<int>(entity->transform.y) - 120 - Game::camera.y;
        banner.w = 400;
        banner.h = 100;

        Renderer::DrawSprite(gift->bannerTexture, SDL_Rect{0, 0, 0, 0}, banner, SDL_FLIP_NONE);

        if (SDL_GetTicks() - gift->openTime > 2000) {
            SDL_Color color = ((SDL_GetTicks() / 500) % 2 == 0)
                ? SDL_Color{255, 215, 0, 255}
                : SDL_Color{255, 255, 255, 255};
            TextRenderer::DrawText(banner.x + 60, banner.y + 110,
                                   "按回车继续旅行，去爱心湖那里看看吧", color);
        }
    }
}
