#include "Views/VillageView.h"
#include <cmath>
#include "Engine/Entity.h"
#include "Engine/Game.h"
#include "Engine/ResourceManager.h"
#include "Engine/SceneManager.h"
#include "Engine/SceneRegistry.h"
#include "Engine/TextRenderer.h"
#include "Views/GiftBanner.h"

void VillageView::OnEnter(SceneContext& context) {
    controller = dynamic_cast<VillageController*>(context.controller);
}

void VillageView::DrawText(const std::string& text, int x, int y, SDL_Color color) {
    TextRenderer::DrawText(x, y, text, color);
}

void VillageView::DrawCutscene(SceneContext& context) {
    if (!controller) return;

    SDL_SetRenderDrawBlendMode(Game::renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(Game::renderer, 0, 20, 0, 220);
    SDL_Rect screen = {0, 0, 800, 600};
    SDL_RenderFillRect(Game::renderer, &screen);

    Entity* player = context.manager->Player();
    if (player) {
        SDL_Rect pRect = player->Bounds();
        SDL_Rect scanBox = {pRect.x - Game::camera.x - 20, pRect.y - Game::camera.y - 30, pRect.w + 40, pRect.h + 50};
        SDL_SetRenderDrawColor(Game::renderer, 0, 255, 0, 255);
        SDL_RenderDrawRect(Game::renderer, &scanBox);

        Uint32 ticks = SDL_GetTicks();
        int scanOffset = static_cast<int>(sin(ticks / 200.0f) * (scanBox.h / 2));
        int lineY = scanBox.y + scanBox.h / 2 + scanOffset;
        SDL_RenderDrawLine(Game::renderer, scanBox.x, lineY, scanBox.x + scanBox.w, lineY);
    }

    int textY = 100;
    SDL_Color green = {0, 255, 0, 255};
    SDL_Color gold = {255, 215, 0, 255};
    for (const auto& line : controller->logLines) {
        SDL_Color color = (line.find("乌拉") != std::string::npos) ? gold : green;
        DrawText(line, 50, textY, color);
        textY += 30;
    }
}

void VillageView::DrawWordCloud(SceneContext& context) {
    if (!controller) return;

    SDL_SetRenderDrawBlendMode(Game::renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(Game::renderer, 0, 0, 0, 200);
    SDL_Rect screen = {0, 0, 800, 600};
    SDL_RenderFillRect(Game::renderer, &screen);

    int cloudAlpha = 255;
    int finalAlpha = 0;
    float timeAfterFormed = controller->cloudFormed
        ? (SDL_GetTicks() - controller->stateStartTime) / 1000.0f - 2.0f
        : 0.0f;

    if (timeAfterFormed > 2.0f) {
        float fade = (timeAfterFormed - 2.0f) / 2.0f;
        if (fade > 1.0f) fade = 1.0f;
        cloudAlpha = static_cast<int>(255 * (1.0f - fade));
        finalAlpha = static_cast<int>(255 * fade);
    }

    if (cloudAlpha > 0) {
        for (const auto& tag : controller->tags) {
            SDL_Color color = tag.color;
            color.a = static_cast<Uint8>(cloudAlpha);
            int offsetX = static_cast<int>(tag.text.length()) * 5;
            DrawText(tag.text, static_cast<int>(tag.currentX) - offsetX, static_cast<int>(tag.currentY), color);
        }
    }

    if (finalAlpha > 0) {
        SDL_Texture* heart = ResourceManager::GetTexture("heart.png");
        if (heart) {
            SDL_SetTextureAlphaMod(heart, static_cast<Uint8>(finalAlpha));
            SDL_Rect heartRect = {300, 170, 200, 200};
            SDL_RenderCopy(Game::renderer, heart, nullptr, &heartRect);
        }
        DrawText("可爱美丽聪明的拉", 250, 400, {255, 215, 0, static_cast<Uint8>(finalAlpha)});
        DrawText("Verified by Zhao", 550, 520, {200, 200, 200, static_cast<Uint8>(finalAlpha)});
    }
}

void VillageView::Render(SceneContext& context) {
    context.manager->DrawWorld();
    DrawGiftBanners(context);

    if (!controller) return;
    if (controller->state == VillageState::Scanning) DrawCutscene(context);
    else if (controller->state == VillageState::WordCloud) DrawWordCloud(context);
}

static SceneRegistry::ViewProxy proxy_village_view("VillageView", []() {
    return new VillageView();
});
