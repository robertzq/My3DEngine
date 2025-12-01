// src/Map/Map.cpp
#include "Map.h"
#include "TextureManager.h"
#include "ResourceManager.h"
#include "Collectible.h"
#include <sstream>

Map::Map() {
    // 1. 加载资源 (建议你后续找对应的像素图替换)
    // 这里为了演示，暂时复用现有资源或假设文件名
    tex_grass   = ResourceManager::GetTexture("grass.png"); // 0
    tex_tree    = ResourceManager::GetTexture("tree.png");  // 1 (你需要加这个图)
    tex_path    = ResourceManager::GetTexture("dirt.png");  // 2 (复用 dirt)
    tex_water   = ResourceManager::GetTexture("water.png"); // 3
    tex_bridge  = ResourceManager::GetTexture("bridge.png");// 4
    tex_wall    = ResourceManager::GetTexture("wall.png");  // 5
    tex_roof    = ResourceManager::GetTexture("roof.png");  // 6
    tex_door    = ResourceManager::GetTexture("door.png");  // 7
    tex_flower  = ResourceManager::GetTexture("flower.png");// 8
    tex_fence   = ResourceManager::GetTexture("fence.png"); // 9

    // 室内
    tex_wood    = ResourceManager::GetTexture("wood.png");     // 10
    tex_inWall  = ResourceManager::GetTexture("inWall.png");   // 11
    tex_rug     = ResourceManager::GetTexture("rug.png");      // 12
    tex_sofa    = ResourceManager::GetTexture("sofa.png");     // 13
    tex_stage   = ResourceManager::GetTexture("stage.png");    // 14
    tex_entrance= ResourceManager::GetTexture("entrance.png"); // 15
    tex_decor   = ResourceManager::GetTexture("decor.png");    // 16

    tex_mic   = ResourceManager::GetTexture("mic.png");    // 17

    srcRect.x = srcRect.y = 0;
    srcRect.w = destRect.w = 32;
    srcRect.h = destRect.h = 32;
}

Map::~Map() {
    // 资源由 ResourceManager 管理，不需在此释放
}

void Map::LoadMap(std::string path) {
    std::string content = ResourceManager::GetTextContent(path);
    if (content.empty()) return;

    std::stringstream mapFile(content);
    std::string line;

    mapData.clear();
    mapWidth = 0;

    while (std::getline(mapFile, line)) {
        // 去除行尾的回车符 (兼容 Windows/Linux)
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
            line.pop_back();
        }
        if (line.empty()) continue; // 跳过空行

        std::vector<int> rowData;

        // --- 智能判断逻辑 ---
        // 检查这一行是否包含空格（或者制表符），如果有，说明是新格式
        bool isSpaceSeparated = (line.find(' ') != std::string::npos || line.find('\t') != std::string::npos);

        if (isSpaceSeparated) {
            // 【新格式】：使用 stringstream 按数字读取 (支持 10, 11, 100...)
            std::stringstream ss(line);
            int val;
            while (ss >> val) {
                rowData.push_back(val);
            }
        } else {
            // 【旧格式】：按字符读取 (只支持 0-9，兼容旧关卡)
            for (char c : line) {
                if (c >= '0' && c <= '9') {
                    rowData.push_back(c - '0');
                }
                // (可选) 如果你以后想在紧凑格式里用 a-z 表示 10-35，可以加上这个：
                // else if (c >= 'a' && c <= 'z') rowData.push_back(c - 'a' + 10);
            }
        }

        if (!rowData.empty()) {
            mapData.push_back(rowData);
            if (rowData.size() > mapWidth) mapWidth = rowData.size();
        }
    }
    mapHeight = mapData.size();
    std::cout << "地图加载成功: " << path << " (" << mapWidth << "x" << mapHeight << ")" << std::endl;
}

void Map::DrawMap() {
    for(int row = 0; row < mapData.size(); row++) {
        for(int col = 0; col < mapData[row].size(); col++) {
            int type = mapData[row][col];

            destRect.x = col * 32 - Game::camera.x;
            destRect.y = row * 32 - Game::camera.y;

            // 视口剔除
            if (destRect.x < -32 || destRect.x > 800 || destRect.y < -32 || destRect.y > 600) continue;

            SDL_Texture* texToDraw = nullptr;
            switch(type) {
                case 0: texToDraw = tex_grass; break;
                case 1: texToDraw = tex_tree; break;
                case 2: texToDraw = tex_path; break;
                case 3: texToDraw = tex_water; break;
                case 4: texToDraw = tex_bridge; break;
                case 5: texToDraw = tex_wall; break;
                case 6: texToDraw = tex_roof; break;
                case 7: texToDraw = tex_door; break;
                case 8: texToDraw = tex_flower; break;
                case 9: texToDraw = tex_fence; break;
                case 10: texToDraw = tex_wood; break;
                case 11: texToDraw = tex_inWall; break;
                case 12: texToDraw = tex_rug; break;
                case 13: texToDraw = tex_sofa; break;
                case 14: texToDraw = tex_stage; break;
                case 15: texToDraw = tex_entrance; break;
                case 16: texToDraw = tex_decor; break;
                case 17: texToDraw = tex_mic; break;
                default: break;
            }
            if(texToDraw)
                TextureManager::DrawWhole(texToDraw, destRect, Game::renderer, SDL_FLIP_NONE);
        }
    }
}

std::vector<SDL_Rect> Map::getColliders() {
    std::vector<SDL_Rect> colliders;

    for(int row = 0; row < mapData.size(); row++) {
        for(int col = 0; col < mapData[row].size(); col++) {
            int type = mapData[row][col];

            // 定义阻挡列表
            // 1=Tree, 3=Water, 5=Wall, 6=Roof(可选), 9=Fence
            // 11=InWall, 13=Sofa, 16=Decor
            bool isBlocking = (type == 1 || type == 3 || type == 5 || type == 6 || type == 9 ||
                               type == 11 || type == 13 );

            if (isBlocking) {
                SDL_Rect collider;
                collider.x = col * 32;
                collider.y = row * 32;
                collider.w = 32;
                collider.h = 32;
                colliders.push_back(collider);
            }
        }
    }
    return colliders;
}

std::vector<SDL_Rect> Map::GetTiles(int tileID) {
    std::vector<SDL_Rect> tiles;
    for(int row = 0; row < mapData.size(); row++) {
        for(int col = 0; col < mapData[row].size(); col++) {
            if (mapData[row][col] == tileID) {
                SDL_Rect r = {col * 32, row * 32, 32, 32};
                tiles.push_back(r);
            }
        }
    }
    return tiles;
}

std::vector<Collectible*> Map::generateHearts() {
    std::vector<Collectible*> generatedHearts;

    for(int row = 0; row < mapData.size(); row++) {
        for(int col = 0; col <  mapData[row].size(); col++) {
            int type = mapData[row][col];

            // 如果发现了 '3' (爱心标记)
            if (type == 3) {
                // 1. 计算坐标
                int x = col * 32;
                int y = row * 32;

                // 2. 创建一个真正的爱心对象
                Collectible* newHeart = new Collectible(x, y);
                generatedHearts.push_back(newHeart);

                // 3. 【关键】把地图上的这个格子抹除！
                // 变成 0 (空气)，这样 Map::DrawMap 就不会在原地再画个东西了
                mapData[row][col] = 0;
            }
        }
    }

    return generatedHearts;
}