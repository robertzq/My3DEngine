#include "Controllers/PlayController.h"
#include "Engine/Game.h"
#include "Engine/Input.h"
#include "Engine/Log.h"
#include "Engine/Physics.h"
#include "Engine/SceneManager.h"
#include "Engine/SceneRegistry.h"

void PlayController::OnEnter(SceneContext& context) {
    SDL_Point spawn = context.manager->SpawnPosition(context.params.value("spawn", "default"), context.params);
    player = new GameObject("redPlayer.png", Game::renderer, spawn.x, spawn.y, 6);
    context.manager->SetPlayer(player);

    giftBox = new GiftBox(2850, 200);

    if (context.map) {
        for (const auto& tile : context.map->TilesWithId(3)) {
            hearts.push_back(new Collectible(tile.x, tile.y));
            context.map->SetTile(tile.x / EngineConfig::TILE_SIZE, tile.y / EngineConfig::TILE_SIZE, 0);
        }
    }
}

void PlayController::OnExit() {
    delete player;
    delete giftBox;
    for (auto* heart : hearts) delete heart;
    hearts.clear();
    player = nullptr;
    giftBox = nullptr;
}

void PlayController::HandleEvent(SceneContext& context, SDL_Event& event) {
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
        player->Jump();
    }
}

void PlayController::Update(SceneContext& context) {
    int dx = 0;
    if (Input::IsKeyDown(SDL_SCANCODE_A)) dx -= 5;
    if (Input::IsKeyDown(SDL_SCANCODE_D)) dx += 5;
    player->SetVelX(dx);
    player->Update();

    if (context.map) {
        Physics::ResolveMapCollision(player, context.map->Colliders());
    }

    if (giftBox && !giftBox->IsOpened()) {
        SDL_Rect playerRect = player->GetBounds();
        SDL_Rect boxRect = giftBox->GetBounds();
        if (Physics::CheckCollision(playerRect, boxRect)) {
            if (player->GetVelY() < 0 && playerRect.y > boxRect.y) {
                giftBox->Open();
                player->LandOnGround(boxRect.y);
            }
        }
    }

    for (auto* heart : hearts) {
        if (!heart->IsCollected()) {
            heart->Update();
            if (Physics::CheckCollision(player->GetBounds(), heart->GetBounds())) {
                heart->Collect();
            }
        }
    }
}

static SceneRegistry::ControllerProxy proxy_play_controller("PlayController", []() {
    return new PlayController();
});
