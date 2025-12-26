#include "BattleScene.h"
#include <iostream>
#include "SceneFactory.h"
#include "ResourceManager.h"
#include "TextureManager.h"
#include "TextRenderer.h"
#include "Game.h"
#include "VillageScene.h"
#include <ctime>

// 全角转换
std::string ToFullWidth(const std::string& input) {
    std::string output = "";
    for (size_t i = 0; i < input.size(); ++i) {
        unsigned char c = (unsigned char)input[i];
        if (c >= 33 && c <= 126) {
            wchar_t codepoint = c + 0xFEE0;
            output += (char)(0xE0 | ((codepoint >> 12) & 0x0F));
            output += (char)(0x80 | ((codepoint >> 6) & 0x3F));
            output += (char)(0x80 | (codepoint & 0x3F));
        } else if (c == 32) {
            output += "\xE3\x80\x80";
        } else {
            output += c;
        }
    }
    return output;
}

BattleScene::BattleScene() {
    bgTexture = ResourceManager::GetTexture("fightbg.png");
    enemyTexture = ResourceManager::GetTexture("enemy.png");
    playerTexture = ResourceManager::GetTexture("playerus.png");
    uiBoxTexture = ResourceManager::GetTexture("dialogrect.png");
    cursorTexture = ResourceManager::GetTexture("point.png");

    maxPlayerHP = 8000; playerHP = 8000;
    maxEnemyHP = 8000; enemyHP = 8000;

    currentState = PLAYER_TURN;
    messageLog = "";
    effectLog = "";
    menuIndex = 0;
    shakeTimer = 0;
    shakeTarget = SHAKE_NONE;
    hugeGiftCD = 0;

    LoadGifts();
}

BattleScene::~BattleScene() {}

void BattleScene::LoadGifts() {
    // 默认兜底数据
    basicGift = {"单推", 99};
    blindBoxGift = {"心动盲盒", 150};
    hugeGift = {"舰长一号", 1980};
    starWishGift = {"星愿水晶球", 1000};

    std::string jsonStr = ResourceManager::GetTextContent("gifts.json");
    if (jsonStr.empty()) {
        std::cout << "Warning: gifts.json not found or empty!" << std::endl;
        enemyGiftPool.push_back(basicGift);
        return;
    }

    try {
        auto j = json::parse(jsonStr);
        if (j.contains("itemList")) {
            blindBoxPool.clear();
            enemyGiftPool.clear();

            for (const auto& item : j["itemList"]) {
                std::string name = item["gname"];
                int battery = item["gbattery"];
                Gift g = {name, battery};

                enemyGiftPool.push_back(g);

                if (name == "单推") basicGift = g;
                else if (name == "心动盲盒") blindBoxGift = g;
                else if (name == "星愿水晶球") starWishGift = g;
                else blindBoxPool.push_back(g);
            }
        }
    } catch (std::exception& e) {
        std::cout << "JSON Parse Error: " << e.what() << std::endl;
    }
}

void BattleScene::OnEnter() {
    std::cout << "进入 PK 战斗模式！" << std::endl;
}

void BattleScene::OnExit() {}

std::string BattleScene::GetDamageText(int damage) {
    if (damage >= 1000) return "效果核爆！！！！！";
    if (damage >= 150) return "效果拔群！！！";
    if (damage >= 50) return "效果显著！";
    return "效果普通";
}

void BattleScene::HandleEvents(SDL_Event& event) {
    if (currentState == PLAYER_TURN) {
        if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                // === 【修改】支持 2x2 网格导航 ===
                case SDLK_UP:
                    // 向上：0和2互换，1和3互换 (-2)
                    menuIndex = (menuIndex - 2 + 4) % 4;
                    break;
                case SDLK_DOWN:
                    // 向下：同理 (+2)
                    menuIndex = (menuIndex + 2) % 4;
                    break;
                case SDLK_LEFT:
                    // 向左：0<->1, 2<->3
                    if (menuIndex % 2 != 0) menuIndex--;
                    else menuIndex++; // 如果在左边按左，去右边 (循环体验)
                    break;
                case SDLK_RIGHT:
                    // 向右
                    if (menuIndex % 2 == 0) menuIndex++;
                    else menuIndex--; // 如果在右边按右，回左边
                    break;

                case SDLK_RETURN:
                case SDLK_SPACE:
                    // 冷却检查 (巨额礼物是 index 1)
                    if (menuIndex == 1 && hugeGiftCD > 0) return;

                    // === 玩家回合逻辑 ===
                    std::string moveName = "";

                    // 选项 0: 单推
                    if (menuIndex == 0) {
                        damageDealt = basicGift.battery;
                        moveName = basicGift.name;
                    }
                    // 选项 1: 舰长一号 (特殊, CD)
                    else if (menuIndex == 1) {
                        damageDealt = hugeGift.battery;
                        moveName = hugeGift.name;
                        hugeGiftCD = MAX_HUGE_GIFT_CD;
                    }
                    // 选项 2: 心动盲盒
                    else if (menuIndex == 2) {
                        if (!blindBoxPool.empty()) {
                            int idx = rand() % blindBoxPool.size();
                            Gift g = blindBoxPool[idx];
                            damageDealt = g.battery;
                            moveName = "盲盒爆出" + g.name;
                        } else {
                            damageDealt = 1;
                            moveName = "盲盒（空）";
                        }
                    }
                    // 选项 3: 星愿水晶球
                    else if (menuIndex == 3) {
                        damageDealt = starWishGift.battery;
                        moveName = starWishGift.name;
                    }

                    enemyHP -= damageDealt;
                    if (enemyHP < 0) enemyHP = 0;

                    {
                        std::string rawMsg = "我方使用 " + moveName + " 造成 " + std::to_string(damageDealt) + "电池 伤害";
                        messageLog = ToFullWidth(rawMsg);
                    }
                    effectLog = ToFullWidth(GetDamageText(damageDealt));

                    shakeTarget = SHAKE_ENEMY;
                    shakeTimer = 20;
                    currentState = PLAYER_ANIM;
                    break;
            }
        }
    } else if (currentState == VICTORY || currentState == DEFEAT) {
        if (event.type == SDL_KEYDOWN) {
            Game::instance()->ChangeScene(new VillageScene("village.map", false));
        }
    }
}

void BattleScene::Update() {
    static int timer = 0;

    if (shakeTimer > 0) {
        shakeTimer--;
        if (shakeTimer <= 0) shakeTarget = SHAKE_NONE;
    }

    if (currentState == PLAYER_ANIM) {
        timer++;
        if (timer > 120) {
            timer = 0;
            if (enemyHP <= 0) {
                currentState = VICTORY;
                messageLog = "胜利！";
                effectLog = "已征服对方主播";
                Game::instance()->bossDefeatedCount +=1;
            } else {
                currentState = ENEMY_TURN;
                messageLog = "";
                effectLog = "";
            }
        }
    }
    else if (currentState == ENEMY_TURN) {
        int dmg = 0;
        std::string enemyMove = "";

        // 5% 概率使用舰长一号
        int roll = rand() % 100;
        if (roll < 25) {
            dmg = hugeGift.battery;
            enemyMove = hugeGift.name;
        }
        else {
            if (!enemyGiftPool.empty()) {
                int idx = rand() % enemyGiftPool.size();
                Gift g = enemyGiftPool[idx];
                dmg = g.battery;
                enemyMove = g.name;
            } else {
                dmg = 10;
                enemyMove = "普通攻击";
            }
        }

        playerHP -= dmg;
        if (playerHP < 0) playerHP = 0;

        {
            std::string rawMsg = "对方使用 " + enemyMove + " 造成 " + std::to_string(dmg) + "电池 伤害";
            messageLog = ToFullWidth(rawMsg);
        }
        effectLog = ToFullWidth(GetDamageText(dmg));

        shakeTarget = SHAKE_PLAYER;
        shakeTimer = 20;
        currentState = ENEMY_ANIM;
    }
    else if (currentState == ENEMY_ANIM) {
        timer++;
        if (timer > 120) {
            timer = 0;
            if (playerHP <= 0) {
                currentState = DEFEAT;
                messageLog = "失败……";
                effectLog = "请重新来过";
            } else {
                currentState = PLAYER_TURN;
                messageLog = "";
                effectLog = "";
                if (hugeGiftCD > 0) hugeGiftCD--;
            }
        }
    }
}

void BattleScene::DrawHPBar(int x, int y, int current, int max, SDL_Color color) {
    SDL_Rect bgRect = {x, y, 200, 20};
    SDL_SetRenderDrawColor(Game::renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(Game::renderer, &bgRect);

    if (max <= 0) max = 1;
    float percent = (float)current / max;
    if (percent < 0) percent = 0;

    SDL_Rect fgRect = {x + 2, y + 2, (int)((196) * percent), 16};
    SDL_SetRenderDrawColor(Game::renderer, color.r, color.g, color.b, 255);
    SDL_RenderFillRect(Game::renderer, &fgRect);

    SDL_SetRenderDrawColor(Game::renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(Game::renderer, &bgRect);
}

void BattleScene::Render() {
    if (bgTexture) {
        TextureManager::DrawWhole(bgTexture, {0, 0, 800, 600}, Game::renderer, SDL_FLIP_NONE);
    } else {
        SDL_SetRenderDrawColor(Game::renderer, 0, 0, 0, 255);
        SDL_RenderClear(Game::renderer);
    }

    int shakeX = 0, shakeY = 0;
    if (shakeTimer > 0) {
        shakeX = (rand() % 10) - 5;
        shakeY = (rand() % 10) - 5;
    }

    // 画敌人
    if (enemyTexture) {
        SDL_Rect enemyRect = {500, 100, 128, 128};
        if (shakeTarget == SHAKE_ENEMY) {
            enemyRect.x += shakeX; enemyRect.y += shakeY;
        }
        TextureManager::DrawWhole(enemyTexture, enemyRect, Game::renderer, SDL_FLIP_NONE);
    }
    DrawHPBar(464, 60, enemyHP, maxEnemyHP, {220, 50, 50, 255});

    // 画我方
    if (playerTexture) {
        SDL_Rect playerRect = {150, 300, 128, 128};
        if (shakeTarget == SHAKE_PLAYER) {
            playerRect.x += shakeX; playerRect.y += shakeY;
        }
        TextureManager::DrawWhole(playerTexture, playerRect, Game::renderer, SDL_FLIP_NONE);
    }
    DrawHPBar(114, 260, playerHP, maxPlayerHP, {50, 220, 50, 255});

    // 画 UI
    SDL_Rect uiRect = {0, 450, 800, 150};
    if (uiBoxTexture) {
        TextureManager::DrawWhole(uiBoxTexture, uiRect, Game::renderer, SDL_FLIP_NONE);
    } else {
        SDL_SetRenderDrawColor(Game::renderer, 0, 0, 0, 200);
        SDL_RenderFillRect(Game::renderer, &uiRect);
    }

    // === 【修改】渲染文字与 2x2 布局 ===

    // 起始位置
    int startX = 60;
    int startY = 480;

    // 间距
    int colGap = 300; // 列间距
    int rowGap = 50;  // 行间距 (上下两行)

    SDL_Color black  = {0, 0, 0, 255};
    SDL_Color darkYellow = {218, 165, 32, 255};
    SDL_Color red    = {255, 50, 50, 255};
    SDL_Color green  = {50, 200, 50, 255};
    SDL_Color gray   = {128, 128, 128, 255};
    SDL_Color menuColor = black;

    if (currentState == PLAYER_TURN) {
        // 第一行 (Y = startY)
        // 0. 单推 (左上)
        TextRenderer::DrawText(startX, startY, basicGift.name, menuColor);

        // 1. 舰长一号 (右上)
        if (hugeGiftCD > 0) {
            std::string cdText = "冷却中(" + std::to_string(hugeGiftCD) + ")";
            TextRenderer::DrawText(startX + colGap, startY, ToFullWidth(cdText), gray);
        } else {
            TextRenderer::DrawText(startX + colGap, startY, hugeGift.name, menuColor);
        }

        // 第二行 (Y = startY + rowGap)
        // 2. 心动盲盒 (左下)
        TextRenderer::DrawText(startX, startY + rowGap, blindBoxGift.name, menuColor);

        // 3. 星愿水晶球 (右下)
        TextRenderer::DrawText(startX + colGap, startY + rowGap, starWishGift.name, menuColor);

        // 渲染光标
        if (cursorTexture) {
            // 计算光标坐标
            // 索引 0, 2 在左列 (col=0); 1, 3 在右列 (col=1)
            int col = menuIndex % 2;
            // 索引 0, 1 在第一行 (row=0); 2, 3 在第二行 (row=1)
            int row = menuIndex / 2;

            int cursorX = startX + (col * colGap);
            int cursorY = startY + (row * rowGap);

            // 让光标在文字左边一点，居中一点
            SDL_Rect cursorRect = {cursorX - 45, cursorY + 5, 40, 40};
            TextureManager::DrawWhole(cursorTexture, cursorRect, Game::renderer, SDL_FLIP_NONE);
        }
    } else {
        // 战斗信息
        if (currentState == VICTORY) TextRenderer::DrawText(350, 200, "胜利！", darkYellow);
        else if (currentState == DEFEAT) TextRenderer::DrawText(350, 200, "失败！", red);

        if (!messageLog.empty()) TextRenderer::DrawText(50, startY, messageLog, menuColor);
        if (!effectLog.empty()) {
            SDL_Color c = green;
            if (effectLog.find("拔群") != std::string::npos) c = red;
            else if (effectLog.find("核爆") != std::string::npos) c = red;
            else if (effectLog.find("显著") != std::string::npos) c = darkYellow;
            TextRenderer::DrawText(50, startY + 50, effectLog, c);
        }
    }
}

static SceneFactory::Proxy proxy_battle("BattleScene", [](const json& params) {
    return new BattleScene();
});