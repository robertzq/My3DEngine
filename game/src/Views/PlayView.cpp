#include "Views/PlayView.h"
#include "Engine/Game.h"
#include "Engine/SceneRegistry.h"

void PlayView::OnEnter(SceneContext& context) {
    controller = static_cast<PlayController*>(context.controller);
}

void PlayView::Render(SceneContext& context) {
    if (context.map) context.map->Draw(Game::renderer, Game::camera);
    if (!controller) return;

    if (controller->player) controller->player->Render();
    if (controller->giftBox) controller->giftBox->Render();
    for (auto* heart : controller->hearts) heart->Render();
}

static SceneRegistry::ViewProxy proxy_play_view("PlayView", []() {
    return new PlayView();
});
