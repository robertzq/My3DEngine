// src/Scenes/VillageScene.cpp
#include "VillageScene.h"
#include "Game.h"
#include "Physics.h"
#include "ResourceManager.h"
#include "SceneFactory.h"
#include "RPGPlayer.h"
#include "TextRenderer.h"

VillageScene::VillageScene(std::string mapFile, bool spawnAtEntrance)
    : mapPath(mapFile), shouldSpawnAtEntrance(spawnAtEntrance) {}

void VillageScene::OnEnter() {
    std::cout << "进入地图: " << mapPath << std::endl;

    // 1. 加载地图
    map = new Map();
    map->LoadMap(mapPath);

    // 2. 确定主角出生位置
    int startX = 100;
    int startY = 100;
// === 【核心修复】优先级 1: 如果是回村庄，且有记忆坐标 ===
    if (mapPath.find("village") != std::string::npos && Game::instance()->lastVillageX != -1) {

        startX = Game::instance()->lastVillageX;
        startY = Game::instance()->lastVillageY;

        // 【非常重要】必须偏移一点位置！
        // 因为你原来是在“撞到门”的时候记录的坐标。
        // 如果直接放回原处，你一出生就会再次撞门 -> 再次进屋 -> 陷入死循环！
        // 假设门都在房子上方，我们让主角向下挪 50 像素
        startY += 50;

        // (可选) 读取完后，可以重置它，也可以保留
        // Game::instance()->lastVillageX = -1;
    }
    // 如果指定了出生在入口 (ID: 15)，则扫描地图
    else if (shouldSpawnAtEntrance) {
        std::vector<SDL_Rect> entrances = map->GetTiles(15);
        if (!entrances.empty()) {
            startX = entrances[0].x;
            startY = entrances[0].y;
            // 稍微往上一点，不要卡在墙里
            startY -= 70;
        }
    }

    player = new RPGPlayer("rpgPlayer.png", Game::renderer, startX, startY, 4, 4);

    // 3. 加载触发器
    // ID 7: 门 (Village -> House)
    doorTriggers = map->GetTiles(7);

    // ID 14: 舞台/Boss (House -> Battle)
    bossTriggers = map->GetTiles(17);

    exitTriggers = map->GetTiles(15);  // 【新增】出门 (室内地毯)

    // 初始化状态
    currentState = SceneState::PLAYING;

    // 如果是村庄地图，并且BOSS打够了（这里假设先设为 0 用于测试，等你做完战斗逻辑再改成 3）
    if (mapPath.find("village") != std::string::npos && Game::instance()->bossDefeatedCount >= 3) {

        // 在花圈中心 (大概是第10行, 第14列 -> x=14*32, y=10*32) 生成礼物
        // 简单起见，我暂时先用文字提示你位置
        std::cout << "【彩蛋】礼物盒已出现在广场中心！ 352，320" << std::endl;
        gifts.push_back(new GiftBox(320, 288));
    }
}

void VillageScene::OnExit() {
    delete player;
    delete map;
    // 【新增】清理礼物盒内存
    for (auto g : gifts) {
        delete g;
    }
    gifts.clear();
}

void VillageScene::HandleEvents(SDL_Event& event) {
// 如果在演出中，禁止玩家移动，或者可以按任意键跳过（可选）
    if (currentState != SceneState::PLAYING) {
        return;
    }
    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_w: player->SetVelY(-4); break;
            case SDLK_s: player->SetVelY(4);  break;
            case SDLK_a: player->SetVelX(-4); break;
            case SDLK_d: player->SetVelX(4);  break;
        }

        if (event.key.keysym.sym == SDLK_RETURN) {
                    // 遍历礼物，看看有没有打开的
                    for (auto g : gifts) {
                        if (g->IsOpened() && !g->IsBannerClosed()) {

                            g->closeBanner();
                            std::cout << "玩家收到提示，准备前往爱心湖" << std::endl;
                            player->SetInputEnabled(true);
                        }
                    }
                }
    }
    if (event.type == SDL_KEYUP) {
        switch (event.key.keysym.sym) {
            case SDLK_w:
            case SDLK_s: player->SetVelY(0); break;
            case SDLK_a:
            case SDLK_d: player->SetVelX(0); break;
        }
    }
}

void VillageScene::Update() {
    if (currentState == SceneState::PLAYING) {
        player->Update();

        // 1. 墙壁碰撞
        std::vector<SDL_Rect> obstacles = map->getColliders();
        Physics::ResolveRPGCollision(player, obstacles);

        // 2. 摄像机跟随
        Game::camera.x = player->GetBounds().x - 400;
        Game::camera.y = player->GetBounds().y - 300;
        if(Game::camera.x < 0) Game::camera.x = 0;
        if(Game::camera.y < 0) Game::camera.y = 0;

        // 3. 检测进门 (Village -> House)
        for (const auto& door : doorTriggers) {
            if (Physics::CheckCollision(player->GetBounds(), door)) {
                std::cout << "进入房子..." << std::endl;
                // 切换到 house.map，并要求出生在入口位置 (true)
                std::string targetMap = "house1.map"; // 默认
                Game::instance()->lastVillageX = player->GetBounds().x;
                Game::instance()->lastVillageY = player->GetBounds().y;
                // 简单的根据坐标判断是哪个房子
                // 注意：这里的坐标数值需要你根据 village.map 的实际格子数算一下
                // 例如：上方的房子大概在 y < 300
                if (door.y < 300) {
                    targetMap = "house1.map";
                }
                // 右边的房子 x > 500
                else if (door.x > 500) {
                    targetMap = "house2.map";
                }
                // 剩下的就是左下角的
                else {
                    targetMap = "house3.map";
                }

                std::cout << "进入房子: " << targetMap << std::endl;
                Game::instance()->ChangeScene(new VillageScene(targetMap, true));

                return;
            }
        }
    // --- 4. 【新增】检测出门 (House -> Village) ---
        for (const auto& exit : exitTriggers) {
            if (Physics::CheckCollision(player->GetBounds(), exit)) {
                std::cout << "返回村庄..." << std::endl;
                // 回村庄时，最好出生在房门口，而不是村口。
                // 暂时先 false，或者你可以专门传个参数指定坐标。
                Game::instance()->ChangeScene(new VillageScene("village.map", false));
                return;
            }
        }

        for (auto g : gifts) {
            // 如果碰到了，并且还没打开
            if (!g->IsOpened() && Physics::CheckCollision(player->GetBounds(), g->GetBounds())) {
                std::cout << "触碰礼物盒！生日快乐！" << std::endl;
                g->Open(); // 打开盒子，GiftBox 内部会处理 flag 并在 Render 时画出横幅
                player->SetInputEnabled(false);
            }
        }
        // --- 5. 检测 BOSS ---
        for (const auto& bossZone : bossTriggers) {
            if (Physics::CheckCollision(player->GetBounds(), bossZone)) {
                std::cout << "舞台触发！BOSS 战！" << std::endl;
                // 1. 准备参数
                        json params;

                        // 根据当前的地图文件名，决定打哪个 BOSS
                        if (mapPath.find("house1.map") != std::string::npos) {
                            params["enemy_id"] = "boss_slime"; // 1号房 BOSS
                        } else if (mapPath.find("house2.map") != std::string::npos) {
                            params["enemy_id"] = "boss_goblin"; // 2号房 BOSS
                        } else {
                            params["enemy_id"] = "boss_king";   // 3号房 BOSS
                        }

                        // 2. 传递当前玩家位置，以便打完仗回来还能站在这
                        params["return_map"] = mapPath;
                        params["return_x"] = player->GetBounds().x;
                        params["return_y"] = player->GetBounds().y + 32; // 回来时站在舞台下方，防止死循环

                        // 3. 切换场景
                        // 注意：SceneFactory::Create 可能需要你注册 BattleScene
                        Scene* battle = SceneFactory::Create("BattleScene", params);
                        if (battle) {
                            Game::instance()->ChangeScene(battle);
                            return; // 马上 return，停止当前场景的 Update，防止漂移
                        }
            }
        }

                // *** 触发器：检测是否到达爱心湖 ***
                // 地图数据中爱心湖在 (Col 23, Row 15) 到 (Col 27, Row 18)
                // 坐标约为 x:736, y:480。
                // 我们把触发区域设得比湖稍微大一点 (向外扩充 20 像素)，
                // 这样主角站在岸边就能触发。
                SDL_Rect lakeTrigger = {
                    736 - 20,  // x: 716
                    480 - 20,  // y: 460
                    160 + 40,  // w: 200 (覆盖整个爱心宽度 + 两边岸边)
                    128 + 40   // h: 168 (覆盖整个爱心高度 + 上下岸边)
                };
                if (Physics::CheckCollision(player->GetBounds(), lakeTrigger)) {
                    TriggerScanSequence(); // 触发演出
                    return;
                }
    } else if (currentState == SceneState::SCANNING) {
        Uint32 currentTime = SDL_GetTicks();
        float elapsed = (currentTime - sequenceStartTime) / 1000.0f;

        // 脚本节奏控制
        if (elapsed > 0.5f && scanStage == 0) {
            logLines.push_back("[SYSTEM] Proximity Alert Triggered.");
            scanStage++;
        }
        else if (elapsed > 1.5f && scanStage == 1) {
            logLines.push_back("[KERNEL] Loading Model: MobileNetV3 (Embedded)..."); // 炫技点1
            scanStage++;
        }
        else if (elapsed > 2.5f && scanStage == 2) {
            logLines.push_back("[AI] Allocating NPU Resources... OK."); // 炫技点2
            scanStage++;
        }
        else if (elapsed > 3.5f && scanStage == 3) {
            logLines.push_back("[VISION] Analyzing Biometrics...");
            scanStage++;
        }
        else if (elapsed > 5.0f && scanStage == 4) {
            logLines.push_back("[RESULT] Match Found: Confidence 99.98%");
            scanStage++;
        }
        else if (elapsed > 6.0f && scanStage == 5) {
            logLines.push_back("[AUTH] Welcome back, 乌拉."); // 情感点
            scanStage++;
        }
        else if (elapsed > 8.0f) {
            // 扫描结束，进入词云
            InitCloudSequence();
            currentState = SceneState::SHOW_WORD_CLOUD;
            sequenceStartTime = SDL_GetTicks(); // 重置计时器给下一个阶段
        }
    } else if (currentState == SceneState::SHOW_WORD_CLOUD) {
            Uint32 currentTime = SDL_GetTicks();
            float elapsed = (currentTime - sequenceStartTime) / 1000.0f;

            // 动画：从中心飞向心形目标位置 (持续 2 秒)
            float duration = 2.0f;
            float t = elapsed / duration;
            if (t > 1.0f) {
                t = 1.0f;
                isCloudFormed = true;
            }
            // 缓动函数 (Ease Out Cubic)
            float easeT = 1.0f - pow(1.0f - t, 3.0f);

            int centerX = 400; // 屏幕中心
            int centerY = 300;

            for (auto& tag : tags) {
                // 插值计算当前位置
                tag.currentX = centerX + (tag.targetX - centerX) * easeT;
                tag.currentY = centerY + (tag.targetY - centerY) * easeT;
            }
    }
}

void VillageScene::Render() {
    map->DrawMap();
    player->Render();
    for (auto g : gifts) {
        g->Render();
    }

    // 1. 扫描界面渲染
        if (currentState == SceneState::SCANNING) {
            // 半透明深绿/黑色遮罩
            SDL_SetRenderDrawBlendMode(Game::renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(Game::renderer, 0, 20, 0, 220);
            SDL_Rect screen = {0, 0, 800, 600};
            SDL_RenderFillRect(Game::renderer, &screen);

            // 绘制扫描框 (绿色空心矩形，在玩家周围)
            SDL_SetRenderDrawColor(Game::renderer, 0, 255, 0, 255);
            SDL_Rect pRect = player->GetBounds();
            SDL_Rect scanBox = { pRect.x - Game::camera.x - 20, pRect.y - Game::camera.y - 30, pRect.w + 40, pRect.h + 50 };
            SDL_RenderDrawRect(Game::renderer, &scanBox);

            // 绘制扫描线 (上下移动)
            Uint32 ticks = SDL_GetTicks();
            int scanOffset = (int)(sin(ticks / 200.0f) * (scanBox.h / 2));
            int lineY = scanBox.y + scanBox.h / 2 + scanOffset;
            SDL_RenderDrawLine(Game::renderer, scanBox.x, lineY, scanBox.x + scanBox.w, lineY);

            // 绘制 Log 文字
            int textY = 100;
            SDL_Color green = {0, 255, 0, 255};
            SDL_Color gold = {255, 215, 0, 255};

            for (const auto& line : logLines) {
                // 欢迎语用金色
                SDL_Color c = (line.find("乌拉") != std::string::npos) ? gold : green;
                DrawTextHelper(line, 50, textY, 16, c);
                textY += 30;
            }
        }

        // 2. 词云渲染
        if (currentState == SceneState::SHOW_WORD_CLOUD) {
            // 半透明黑色背景，让心形更突出
            SDL_SetRenderDrawBlendMode(Game::renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(Game::renderer, 0, 0, 0, 200);
            SDL_Rect screen = {0, 0, 800, 600};
            SDL_RenderFillRect(Game::renderer, &screen);

            // 计算透明度逻辑
                // 阶段1: 0~2秒，词云完全显示，FinalText 透明
                // 阶段2: 2~4秒，词云淡出，FinalText 淡入
                // 阶段3: 4秒后，词云消失，FinalText 常驻

                int cloudAlpha = 255;
                int finalAlpha = 0;

                // 假设 cloudDisplayTimer 是你记录词云形成后经过的秒数
                // 你需要在 Update 里维护这个计时器
                // 为了方便，这里我临时用 SDL_GetTicks 算一下，你最好用类成员变量
                float timeAfterFormed = (isCloudFormed) ? (SDL_GetTicks() - sequenceStartTime)/1000.0f - 2.0f : 0.0f;
                // 注意：这里的 logic 是假设 飞行动画 耗时 2秒。

                if (timeAfterFormed > 2.0f) { // 停留2秒后开始转场
                    float fadeProgress = (timeAfterFormed - 2.0f) / 2.0f; // 2秒的转场时间
                    if (fadeProgress > 1.0f) fadeProgress = 1.0f;

                    cloudAlpha = (int)(255 * (1.0f - fadeProgress)); // 慢慢变透明
                    finalAlpha = (int)(255 * fadeProgress);          // 慢慢变实
                }

                // --- 绘制词云 (带透明度) ---
                if (cloudAlpha > 0) {
                    for (const auto& tag : tags) {
                        SDL_Color c = tag.color;
                        c.a = cloudAlpha; // 修改透明度

                        // 简单的居中偏移
                        int offsetX = tag.text.length() * 5;
                        DrawTextHelper(tag.text, (int)tag.currentX - offsetX, (int)tag.currentY, tag.fontSize, c);
                    }
                }

                // --- 绘制最终结局 (带透明度) ---
                if (finalAlpha > 0) {
                    // 1. 画一个大大的粉色爱心作为背景
                    // 假设你有一个 heart.png 加载到了 tex_heart (或者临时加载)
                    // TextureManager::Draw...
                    // 如果没有素材，就不画，或者用文字画个大爱心符号

                    SDL_Texture* bigHeart = ResourceManager::GetTexture("heart.png"); // 你资源里好像有这个
                    if (bigHeart) {
                        SDL_SetTextureAlphaMod(bigHeart, finalAlpha); // 设置纹理透明度
                        SDL_Rect heartRect = {400 - 100, 300 - 100 - 30, 200, 200}; // 居中放
                        SDL_RenderCopy(Game::renderer, bigHeart, NULL, &heartRect);
                    }

                    // 2. 画最终文字
                    SDL_Color gold = {255, 215, 0, (Uint8)finalAlpha};
                    DrawTextHelper("聪明美丽可爱的拉", 250, 500 - 100, 32, gold);

                    SDL_Color white = {200, 200, 200, (Uint8)finalAlpha};
                    DrawTextHelper("Verified by Zhao", 550, 550 - 30, 14, white);
                }
        }
}
// --- 辅助逻辑实现 ---

void VillageScene::TriggerScanSequence() {
    currentState = SceneState::SCANNING;
    sequenceStartTime = SDL_GetTicks();
    logLines.clear();
    scanStage = 0;
    // 停止玩家移动
    player->SetVelX(0);
    player->SetVelY(0);
    std::cout << ">>> 安全扫描启动 <<<" << std::endl;
}

void VillageScene::InitCloudSequence() {
    tags.clear();

    // 1. 词库 (按照你要求的“非煽情、数据化”风格)
    struct WordInfo { std::string text; int size; };
    std::vector<WordInfo> words = {
            {"75w粉", 24}, {"时尚UP主", 24}, {"上海囡囡", 20}, {"懂穿搭", 20},
            {"全能策划", 20}, {"软妹子", 18}, {"细节控", 18}, {"拖延症患者", 18},
            {"脾气好", 18}, {"关东煮品鉴师", 16}, {"社死的甲方大人", 16},
            {"鱿鱼小姐", 22}, {"乌拉乌拉怪xx", 22}, {"爱播Wula", 22} // 注意这里改成英文逗号
        };

    int centerX = 400;
    int centerY = 280; // 稍微靠上一点
    // --- 核心算法修改：双层分布 ---

        // 我们手动把词分成两组：内圈(6个) 和 外圈(8个)
        int innerCount = 6;

        for (size_t i = 0; i < words.size(); i++) {
            float scale;
            float t;

            if (i < innerCount) {
                // --- 内圈设置 ---
                // 只有6个词，放在较小的心形上 (缩放系数 7.0)
                scale = 7.0f;
                // t 从 0 到 2PI 均匀分布
                t = (float)i / innerCount * 6.28318f;
            } else {
                // --- 外圈设置 ---
                // 剩下的词，放在较大的心形上 (缩放系数 14.0)
                scale = 14.0f;
                // 重新计算 t，让外圈也均匀分布
                t = (float)(i - innerCount) / (words.size() - innerCount) * 6.28318f;
            }

            // 心形方程
            float x = 16 * pow(sin(t), 3);
            float y = -(13 * cos(t) - 5 * cos(2 * t) - 2 * cos(3 * t) - cos(4 * t));

            CloudTag tag;
            tag.text = words[i].text;

            // 计算目标位置
            tag.targetX = centerX + (int)(x * scale);
            tag.targetY = centerY + (int)(y * scale);

            // 稍微加一点随机偏移，让它看起来更自然，不要排得太死板
            // tag.targetX += (rand() % 10 - 5);
            // tag.targetY += (rand() % 10 - 5);

            tag.fontSize = words[i].size;

            // 颜色分配
            if (i % 3 == 0) tag.color = {255, 105, 180, 255}; // HotPink
            else if (i % 3 == 1) tag.color = {255, 182, 193, 255}; // LightPink
            else tag.color = {221, 160, 221, 255}; // Plum

            // 动画起始点：屏幕中心
            tag.currentX = (float)centerX;
            tag.currentY = (float)centerY;

            tags.push_back(tag);
        }
}

void VillageScene::DrawTextHelper(std::string text, int x, int y, int size, SDL_Color color) {
    // 这里需要调用 TextRenderer。
    // 假设 TextRenderer 是一个单例或者有静态方法。
    // 如果没有，你需要确保在这里能够访问到 TextRenderer 的实例。
    // 下面是一个基于常见 SDL_ttf 封装的调用示例：

    // 确保 assets 目录下有字体文件
    TextRenderer::DrawText( x, y, text, color);
}
// 注册工厂
static SceneFactory::Proxy proxy_Village("VillageScene", [](const json& params) {
    std::string mapPath = params.value("map_file", "village.map");
    // 默认不强制出生在入口，除非参数指定
    bool spawnAtEnt = params.value("spawn_at_entrance", false);
    return new VillageScene(mapPath, spawnAtEnt);
});