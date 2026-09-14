#include "Controllers/BattleController.h"
#include <cstdlib>
#include "Engine/Game.h"
#include "Engine/Log.h"
#include "Engine/ResourceManager.h"
#include "Engine/SceneManager.h"
#include "Engine/SceneRegistry.h"
#include "GameState.h"

namespace {

std::string ToFullWidth(const std::string& input) {
    std::string output;
    for (unsigned char c : input) {
        if (c >= 33 && c <= 126) {
            wchar_t codepoint = c + 0xFEE0;
            output += static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F));
            output += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            output += static_cast<char>(0x80 | (codepoint & 0x3F));
        } else if (c == 32) {
            output += "\xE3\x80\x80";
        } else {
            output += static_cast<char>(c);
        }
    }
    return output;
}

}

void BattleController::OnEnter(SceneContext& context) {
    returnScene = context.params.value("return_scene", "village");
    returnX = context.params.value("spawn_x", -1);
    returnY = context.params.value("spawn_y", -1);

    bgTexture = ResourceManager::GetTexture("fightbg.png");
    enemyTexture = ResourceManager::GetTexture("enemy.png");
    playerTexture = ResourceManager::GetTexture("playerus.png");
    uiBoxTexture = ResourceManager::GetTexture("dialogrect.png");
    cursorTexture = ResourceManager::GetTexture("point.png");

    currentState = PLAYER_TURN;
    messageLog.clear();
    effectLog.clear();
    menuIndex = 0;
    shakeTimer = 0;
    shakeTarget = SHAKE_NONE;
    hugeGiftCD = 0;
    animTimer = 0;

    LoadGifts();
    LOG_INFO("进入 PK 战斗模式！");
}

void BattleController::OnExit() {}

void BattleController::LoadGifts() {
    basicGift = {"单推", 99};
    blindBoxGift = {"心动盲盒", 150};
    hugeGift = {"舰长一号", 1980};
    starWishGift = {"星愿水晶球", 1000};

    std::string jsonStr = ResourceManager::GetTextContent("gifts.json");
    if (jsonStr.empty()) {
        LOG_WARN("gifts.json not found or empty!");
        enemyGiftPool.push_back(basicGift);
        return;
    }

    try {
        auto j = json::parse(jsonStr);
        if (j.contains("itemList")) {
            blindBoxPool.clear();
            enemyGiftPool.clear();
            for (const auto& item : j["itemList"]) {
                Gift gift = {item["gname"], item["gbattery"]};
                enemyGiftPool.push_back(gift);
                if (gift.name == "单推") basicGift = gift;
                else if (gift.name == "心动盲盒") blindBoxGift = gift;
                else if (gift.name == "星愿水晶球") starWishGift = gift;
                else blindBoxPool.push_back(gift);
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("gifts.json 解析失败: " << e.what());
    }
}

std::string BattleController::DamageText(int damage) const {
    if (damage >= 1000) return "效果核爆！！！！！";
    if (damage >= 150) return "效果拔群！！！";
    if (damage >= 50) return "效果显著！";
    return "效果普通";
}

void BattleController::LeaveBattle(SceneContext& context) {
    json params;
    if (returnX >= 0 && returnY >= 0) {
        params["spawn_x"] = returnX;
        params["spawn_y"] = returnY;
    }
    context.manager->RequestScene(returnScene, "default", params);
}

void BattleController::HandleEvent(SceneContext& context, SDL_Event& event) {
    if (event.type != SDL_KEYDOWN) return;

    if (currentState == PLAYER_TURN) {
        int damage = 0;
        std::string moveName;
        switch (event.key.keysym.sym) {
            case SDLK_UP: menuIndex = (menuIndex - 2 + 4) % 4; break;
            case SDLK_DOWN: menuIndex = (menuIndex + 2) % 4; break;
            case SDLK_LEFT: menuIndex = (menuIndex % 2 != 0) ? menuIndex - 1 : menuIndex + 1; break;
            case SDLK_RIGHT: menuIndex = (menuIndex % 2 == 0) ? menuIndex + 1 : menuIndex - 1; break;
            case SDLK_RETURN:
            case SDLK_SPACE:
                if (menuIndex == 1 && hugeGiftCD > 0) return;
                if (menuIndex == 0) {
                    damage = basicGift.battery;
                    moveName = basicGift.name;
                } else if (menuIndex == 1) {
                    damage = hugeGift.battery;
                    moveName = hugeGift.name;
                    hugeGiftCD = MAX_HUGE_GIFT_CD;
                } else if (menuIndex == 2) {
                    if (!blindBoxPool.empty()) {
                        Gift gift = blindBoxPool[rand() % blindBoxPool.size()];
                        damage = gift.battery;
                        moveName = "盲盒爆出" + gift.name;
                    } else {
                        damage = 1;
                        moveName = "盲盒（空）";
                    }
                } else {
                    damage = starWishGift.battery;
                    moveName = starWishGift.name;
                }

                enemyHP -= damage;
                if (enemyHP < 0) enemyHP = 0;
                messageLog = ToFullWidth("我方使用 " + moveName + " 造成 " + std::to_string(damage) + "电池 伤害");
                effectLog = ToFullWidth(DamageText(damage));
                shakeTarget = SHAKE_ENEMY;
                shakeTimer = 20;
                currentState = PLAYER_ANIM;
                break;
            default: break;
        }
    } else if (currentState == VICTORY || currentState == DEFEAT) {
        LeaveBattle(context);
    }
}

void BattleController::Update(SceneContext& context) {
    if (shakeTimer > 0 && --shakeTimer <= 0) shakeTarget = SHAKE_NONE;

    if (currentState == PLAYER_ANIM) {
        if (++animTimer > 120) {
            animTimer = 0;
            if (enemyHP <= 0) {
                currentState = VICTORY;
                messageLog = "胜利！";
                effectLog = "已征服对方主播";
                g_gameState.bossDefeatedCount += 1;
            } else {
                currentState = ENEMY_TURN;
                messageLog.clear();
                effectLog.clear();
            }
        }
    } else if (currentState == ENEMY_TURN) {
        int damage;
        std::string enemyMove;
        if (rand() % 100 < 25) {
            damage = hugeGift.battery;
            enemyMove = hugeGift.name;
        } else if (!enemyGiftPool.empty()) {
            Gift gift = enemyGiftPool[rand() % enemyGiftPool.size()];
            damage = gift.battery;
            enemyMove = gift.name;
        } else {
            damage = 10;
            enemyMove = "普通攻击";
        }

        playerHP -= damage;
        if (playerHP < 0) playerHP = 0;
        messageLog = ToFullWidth("对方使用 " + enemyMove + " 造成 " + std::to_string(damage) + "电池 伤害");
        effectLog = ToFullWidth(DamageText(damage));
        shakeTarget = SHAKE_PLAYER;
        shakeTimer = 20;
        currentState = ENEMY_ANIM;
    } else if (currentState == ENEMY_ANIM) {
        if (++animTimer > 120) {
            animTimer = 0;
            if (playerHP <= 0) {
                currentState = DEFEAT;
                messageLog = "失败……";
                effectLog = "请重新来过";
            } else {
                currentState = PLAYER_TURN;
                messageLog.clear();
                effectLog.clear();
                if (hugeGiftCD > 0) hugeGiftCD--;
            }
        }
    }
}

static SceneRegistry::ControllerProxy proxy_battle_controller("BattleController", []() {
    return new BattleController();
});
