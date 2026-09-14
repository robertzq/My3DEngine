#include "Views/BattleView.h"
#include "Engine/ResourceManager.h"
#include "Engine/Renderer.h"
#include "Engine/SceneRegistry.h"
#include "Engine/TextRenderer.h"

void BattleView::OnEnter(SceneContext& context) {
    controller = static_cast<BattleController*>(context.controller);
}

void BattleView::DrawHPBar(int x, int y, int current, int max, SDL_Color color) {
    SDL_Rect bg = {x, y, 200, 20};
    Renderer::DrawRect(bg, {40, 40, 40, 255});

    if (max <= 0) max = 1;
    float percent = static_cast<float>(current) / max;
    if (percent < 0) percent = 0;

    SDL_Rect fg = {x + 2, y + 2, static_cast<int>(196 * percent), 16};
    Renderer::DrawRect(fg, color);

    Renderer::DrawRectOutline(bg, {255, 255, 255, 255});
}

void BattleView::Render(SceneContext& context) {
    if (!controller) return;

    if (controller->bgTexture) {
        ResourceManager::DrawWhole(controller->bgTexture, {0, 0, 800, 600}, SDL_FLIP_NONE);
    } else {
        Renderer::Clear({0, 0, 0, 255});
    }

    int shakeX = 0, shakeY = 0;
    if (controller->shakeTimer > 0) {
        shakeX = (rand() % 10) - 5;
        shakeY = (rand() % 10) - 5;
    }

    if (controller->enemyTexture) {
        SDL_Rect enemy = {500, 100, 128, 128};
        if (controller->shakeTarget == SHAKE_ENEMY) { enemy.x += shakeX; enemy.y += shakeY; }
        ResourceManager::DrawWhole(controller->enemyTexture, enemy, SDL_FLIP_NONE);
    }
    DrawHPBar(464, 60, controller->enemyHP, controller->maxEnemyHP, {220, 50, 50, 255});

    if (controller->playerTexture) {
        SDL_Rect player = {150, 300, 128, 128};
        if (controller->shakeTarget == SHAKE_PLAYER) { player.x += shakeX; player.y += shakeY; }
        ResourceManager::DrawWhole(controller->playerTexture, player, SDL_FLIP_NONE);
    }
    DrawHPBar(114, 260, controller->playerHP, controller->maxPlayerHP, {50, 220, 50, 255});

    SDL_Rect ui = {0, 450, 800, 150};
    if (controller->uiBoxTexture) {
        ResourceManager::DrawWhole(controller->uiBoxTexture, ui, SDL_FLIP_NONE);
    } else {
        Renderer::DrawRect(ui, {0, 0, 0, 200});
    }

    int startX = 60;
    int startY = 480;
    int colGap = 300;
    int rowGap = 50;

    SDL_Color black = {0, 0, 0, 255};
    SDL_Color darkYellow = {218, 165, 32, 255};
    SDL_Color red = {255, 50, 50, 255};
    SDL_Color green = {50, 200, 50, 255};
    SDL_Color gray = {128, 128, 128, 255};

    if (controller->currentState == BattleController::PLAYER_TURN) {
        TextRenderer::DrawText(startX, startY, controller->basicGift.name, black);
        if (controller->hugeGiftCD > 0) {
            TextRenderer::DrawText(startX + colGap, startY,
                                   "冷却中(" + std::to_string(controller->hugeGiftCD) + ")", gray);
        } else {
            TextRenderer::DrawText(startX + colGap, startY, controller->hugeGift.name, black);
        }
        TextRenderer::DrawText(startX, startY + rowGap, controller->blindBoxGift.name, black);
        TextRenderer::DrawText(startX + colGap, startY + rowGap, controller->starWishGift.name, black);

        if (controller->cursorTexture) {
            int col = controller->menuIndex % 2;
            int row = controller->menuIndex / 2;
            SDL_Rect cursor = {startX + col * colGap - 45, startY + row * rowGap + 5, 40, 40};
            ResourceManager::DrawWhole(controller->cursorTexture, cursor, SDL_FLIP_NONE);
        }
    } else {
        if (controller->currentState == BattleController::VICTORY) {
            TextRenderer::DrawText(350, 200, "胜利！", darkYellow);
        } else if (controller->currentState == BattleController::DEFEAT) {
            TextRenderer::DrawText(350, 200, "失败！", red);
        }
        if (!controller->messageLog.empty()) {
            TextRenderer::DrawText(50, startY, controller->messageLog, black);
        }
        if (!controller->effectLog.empty()) {
            SDL_Color color = green;
            if (controller->effectLog.find("拔群") != std::string::npos) color = red;
            else if (controller->effectLog.find("核爆") != std::string::npos) color = red;
            else if (controller->effectLog.find("显著") != std::string::npos) color = darkYellow;
            TextRenderer::DrawText(50, startY + 50, controller->effectLog, color);
        }
    }
}

static SceneRegistry::ViewProxy proxy_battle_view("BattleView", []() {
    return new BattleView();
});
