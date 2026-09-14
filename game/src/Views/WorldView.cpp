#include "Views/WorldView.h"
#include "Engine/SceneManager.h"
#include "Engine/SceneRegistry.h"

void WorldView::Render(SceneContext& context) {
    context.manager->DrawWorld();
}

static SceneRegistry::ViewProxy proxy_world_view("WorldView", []() {
    return new WorldView();
});
