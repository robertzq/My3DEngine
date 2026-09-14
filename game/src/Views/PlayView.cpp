#include "Views/PlayView.h"
#include "Engine/SceneManager.h"
#include "Engine/SceneRegistry.h"
#include "Views/GiftBanner.h"

void PlayView::Render(SceneContext& context) {
    context.manager->DrawWorld();
    DrawGiftBanners(context);
}

static SceneRegistry::ViewProxy proxy_play_view("PlayView", []() {
    return new PlayView();
});
