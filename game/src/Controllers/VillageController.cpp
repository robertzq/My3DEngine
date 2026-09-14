#include "Controllers/VillageController.h"
#include <cmath>
#include "Behaviors/GiftBoxBehavior.h"
#include "Behaviors/PlayerBehavior.h"
#include "Engine/Config.h"
#include "Engine/Entity.h"
#include "Engine/Input.h"
#include "Engine/Log.h"
#include "Engine/Physics.h"
#include "Engine/SceneManager.h"
#include "Engine/SceneRegistry.h"
#include "GameState.h"

namespace {

PlayerBehavior* PlayerBehaviorOf(SceneContext& context) {
    Entity* player = context.manager->Player();
    if (!player || !player->behavior) return nullptr;
    return dynamic_cast<PlayerBehavior*>(player->behavior.get());
}

}

void VillageController::OnEnter(SceneContext& context) {
    mapId = context.data ? context.data->id : std::string();

    state = VillageState::Playing;
    cloudFormed = false;
    logLines.clear();
    tags.clear();
    scanStage = 0;

    if (mapId == "village" && g_gameState.bossDefeatedCount >= 3) {
        LOG_INFO("彩蛋：礼物盒已出现在广场中心");
        context.manager->Spawn("GiftBox", "gift", 320, 288);
    }
}

void VillageController::OnExit() {}

void VillageController::Update(SceneContext& context) {
    if (state == VillageState::Playing && Input::Pressed("Interact")) {
        for (Entity* entity : context.manager->Entities()) {
            auto* gift = dynamic_cast<GiftBoxBehavior*>(entity->behavior.get());
            if (gift && gift->IsOpened() && !gift->IsBannerClosed()) {
                gift->CloseBanner();
                if (PlayerBehavior* player = PlayerBehaviorOf(context)) player->SetInputEnabled(true);
            }
        }
    }

    Entity* player = context.manager->Player();
    if (!player) return;

    if (state == VillageState::Playing) {
        if (context.map) {
            for (const auto& zone : context.map->TilesWithId(17)) {
                if (Physics::Overlap(*player, zone)) {
                    json params;
                    params["return_scene"] = mapId;
                    params["spawn_x"] = player->Bounds().x;
                    params["spawn_y"] = player->Bounds().y + EngineConfig::TILE_SIZE;
                    context.manager->RequestTransition("boss", params);
                    return;
                }
            }
        }

        if (mapId == "village") {
            SDL_Rect lake = {716, 460, 200, 168};
            if (Physics::Overlap(*player, lake)) {
                StartScan(context);
                return;
            }
        }

        for (Entity* entity : context.manager->Entities()) {
            auto* gift = dynamic_cast<GiftBoxBehavior*>(entity->behavior.get());
            if (!gift || gift->IsOpened()) continue;
            if (Physics::Overlap(*player, *entity)) {
                LOG_INFO("触碰礼物盒！生日快乐！");
                gift->Open();
                if (PlayerBehavior* behavior = PlayerBehaviorOf(context)) behavior->SetInputEnabled(false);
            }
        }
    } else if (state == VillageState::Scanning) {
        float elapsed = (SDL_GetTicks() - stateStartTime) / 1000.0f;

        if (elapsed > 0.5f && scanStage == 0) {
            logLines.push_back("[SYSTEM] Proximity Alert Triggered.");
            scanStage++;
        } else if (elapsed > 1.5f && scanStage == 1) {
            logLines.push_back("[KERNEL] Loading Model: MobileNetV3 (Embedded)...");
            scanStage++;
        } else if (elapsed > 2.5f && scanStage == 2) {
            logLines.push_back("[AI] Allocating NPU Resources... OK.");
            scanStage++;
        } else if (elapsed > 3.5f && scanStage == 3) {
            logLines.push_back("[VISION] Analyzing Biometrics...");
            scanStage++;
        } else if (elapsed > 5.0f && scanStage == 4) {
            logLines.push_back("[RESULT] Match Found: Confidence 99.98%");
            scanStage++;
        } else if (elapsed > 6.0f && scanStage == 5) {
            logLines.push_back("[AUTH] Welcome back, 乌拉.");
            scanStage++;
        } else if (elapsed > 8.0f) {
            FinishScan(context);
        }
    } else if (state == VillageState::WordCloud) {
        float elapsed = (SDL_GetTicks() - stateStartTime) / 1000.0f;
        float t = elapsed / 2.0f;
        if (t > 1.0f) {
            t = 1.0f;
            cloudFormed = true;
        }
        float easeT = 1.0f - std::pow(1.0f - t, 3.0f);

        int centerX = 400;
        int centerY = 300;
        for (auto& tag : tags) {
            tag.currentX = centerX + (tag.targetX - centerX) * easeT;
            tag.currentY = centerY + (tag.targetY - centerY) * easeT;
        }
    }
}

void VillageController::StartScan(SceneContext& context) {
    state = VillageState::Scanning;
    stateStartTime = SDL_GetTicks();
    logLines.clear();
    scanStage = 0;
    if (PlayerBehavior* player = PlayerBehaviorOf(context)) {
        player->SetVelocity(0, 0);
        player->SetInputEnabled(false);
    }
}

void VillageController::FinishScan(SceneContext& context) {
    BuildWordCloud();
    state = VillageState::WordCloud;
    stateStartTime = SDL_GetTicks();
}

void VillageController::BuildWordCloud() {
    tags.clear();

    struct WordInfo { std::string text; int size; };
    std::vector<WordInfo> words = {
        {"75w粉", 24}, {"时尚UP主", 24}, {"上海囡囡", 20}, {"懂穿搭", 20},
        {"全能策划", 20}, {"软妹子", 18}, {"大厨", 18}, {"拖延症患者", 18},
        {"脾气好", 18}, {"关东煮品鉴师", 16}, {"社死的甲方大人", 16},
        {"鱿鱼小姐", 22}, {"乌拉乌拉怪xx", 22}, {"爱播Wula", 22}
    };

    int centerX = 400;
    int centerY = 280;
    int innerCount = 6;

    for (size_t i = 0; i < words.size(); ++i) {
        float scale;
        float t;
        if (static_cast<int>(i) < innerCount) {
            scale = 7.0f;
            t = static_cast<float>(i) / innerCount * 6.28318f;
        } else {
            scale = 14.0f;
            t = static_cast<float>(i - innerCount) / (words.size() - innerCount) * 6.28318f;
        }

        float x = 16 * std::pow(std::sin(t), 3);
        float y = -(13 * std::cos(t) - 5 * std::cos(2 * t) - 2 * std::cos(3 * t) - std::cos(4 * t));

        CloudTag tag;
        tag.text = words[i].text;
        tag.targetX = centerX + static_cast<int>(x * scale);
        tag.targetY = centerY + static_cast<int>(y * scale);
        tag.fontSize = words[i].size;
        tag.currentX = static_cast<float>(centerX);
        tag.currentY = static_cast<float>(centerY);

        if (i % 3 == 0) tag.color = {255, 105, 180, 255};
        else if (i % 3 == 1) tag.color = {255, 182, 193, 255};
        else tag.color = {221, 160, 221, 255};

        tags.push_back(tag);
    }
}

static SceneRegistry::ControllerProxy proxy_village_controller("VillageController", []() {
    return new VillageController();
});
