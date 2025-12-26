// src/Scenes/VillageScene.cpp
#include "VillageScene.h"
#include "Game.h"
#include "Physics.h"
#include "ResourceManager.h"
#include "SceneFactory.h"
#include "RPGPlayer.h"


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

    // 如果指定了出生在入口 (ID: 15)，则扫描地图
    if (shouldSpawnAtEntrance) {
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
    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_w: player->SetVelY(-4); break;
            case SDLK_s: player->SetVelY(4);  break;
            case SDLK_a: player->SetVelX(-4); break;
            case SDLK_d: player->SetVelX(4);  break;
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
}

void VillageScene::Render() {
    map->DrawMap();
    player->Render();
    for (auto g : gifts) {
        g->Render();
    }
}

// 注册工厂
static SceneFactory::Proxy proxy_Village("VillageScene", [](const json& params) {
    std::string mapPath = params.value("map_file", "village.map");
    // 默认不强制出生在入口，除非参数指定
    bool spawnAtEnt = params.value("spawn_at_entrance", false);
    return new VillageScene(mapPath, spawnAtEnt);
});