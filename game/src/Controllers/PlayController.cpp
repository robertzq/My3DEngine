#include "Controllers/PlayController.h"
#include "Behaviors/CollectibleBehavior.h"
#include "Behaviors/GiftBoxBehavior.h"
#include "Engine/Config.h"
#include "Engine/Entity.h"
#include "Engine/Physics.h"
#include "Engine/SceneManager.h"
#include "Engine/SceneRegistry.h"

void PlayController::OnEnter(SceneContext& context) {
    if (context.map) {
        for (const auto& tile : context.map->TilesWithId(3)) {
            context.manager->Spawn("Collectible", "heart", static_cast<float>(tile.x), static_cast<float>(tile.y));
            context.map->SetTile(tile.x / EngineConfig::TILE_SIZE, tile.y / EngineConfig::TILE_SIZE, 0);
        }
    }
    context.manager->Spawn("GiftBox", "gift", 2850, 200);
}

void PlayController::Update(SceneContext& context) {
    Entity* player = context.manager->Player();
    if (!player) return;

    for (Entity* entity : context.manager->Entities()) {
        if (entity == player) continue;

        if (auto* collectible = dynamic_cast<CollectibleBehavior*>(entity->behavior.get())) {
            if (!collectible->collected && Physics::Overlap(*player, *entity)) {
                collectible->collected = true;
                context.manager->Destroy(entity);
            }
        } else if (auto* gift = dynamic_cast<GiftBoxBehavior*>(entity->behavior.get())) {
            if (!gift->IsOpened() && Physics::Overlap(*player, *entity)) {
                gift->Open();
            }
        }
    }
}

static SceneRegistry::ControllerProxy proxy_play_controller("PlayController", []() {
    return new PlayController();
});
