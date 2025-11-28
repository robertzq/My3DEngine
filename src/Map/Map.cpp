#include "Map.h"
#include "TextureManager.h"
#include "Collectible.h"
#include "ResourceManager.h"
#include <sstream> // <--- 【修复1】必须加上这个！

Map::Map() {
    // 使用 ResourceManager 获取纹理 (复用资源)
    dirt = ResourceManager::GetTexture("dirt.png");
    grass = ResourceManager::GetTexture("grass.png");
    water = ResourceManager::GetTexture("water.png");

    srcRect.x = srcRect.y = 0;
    srcRect.w = destRect.w = 32;
    srcRect.h = destRect.h = 32;

    destRect.x = destRect.y = 0;
}

Map::~Map() {
    // 【修复2】删掉了 SDL_DestroyTexture
    // 现在的纹理属于 ResourceManager 托管，Map 只是借用。
    // 如果这里删了，其他地方（比如 GameObject）也用草地贴图的话，程序就会崩。
}

void Map::LoadMap(std::string path) {
    std::string content = ResourceManager::GetTextContent(path);
    if (content.empty()) return;

    std::stringstream mapFile(content);
    std::string line;

    // 【核心逻辑】按行读取，动态 push_back
    mapData.clear();
    mapHeight = 0;
    mapWidth = 0;

    while (std::getline(mapFile, line)) {
        std::vector<int> rowData;

        // 简单处理：过滤回车符 (Windows兼容)
        if (!line.empty() && line.back() == '\r') line.pop_back();

        for (char c : line) {
            if (c >= '0' && c <= '9') {
                rowData.push_back(c - '0');
            }
        }

        // 只有读到了数据才算一行
        if (!rowData.empty()) {
            mapData.push_back(rowData);
            if (rowData.size() > mapWidth) mapWidth = rowData.size(); // 记录最大宽度
        }
    }
    mapHeight = mapData.size();

    std::cout << "地图加载成功! 大小: " << mapHeight << "x" << mapWidth << std::endl;
}

void Map::DrawMap() {
    int type = 0;

    for(int row = 0; row < mapData.size(); row++) {
        for(int col = 0; col < mapData[row].size(); col++) {
            type = mapData[row][col];

            // 1. 计算屏幕坐标 (只减一次！)
            destRect.x = col * 32 - Game::camera.x;
            destRect.y = row * 32 - Game::camera.y;

            // 2. 简单的视口剔除优化 (可选)
            // 既然已经算出了屏幕坐标 destRect.x，直接判断它是否在屏幕范围内即可
            if (destRect.x < -32 || destRect.x > 800 || destRect.y < -32 || destRect.y > 600) {
                continue; // 如果格子在屏幕外，就不画了，省资源
            }

            switch(type) {
                case 0:
                    TextureManager::Draw(water, srcRect, destRect, Game::renderer, SDL_FLIP_NONE);
                    break;
                case 1:
                    TextureManager::Draw(grass, srcRect, destRect, Game::renderer, SDL_FLIP_NONE);
                    break;
                case 2:
                    TextureManager::Draw(dirt, srcRect, destRect, Game::renderer, SDL_FLIP_NONE);
                    break;
                default:
                    break;
            }
        }
    }
}

std::vector<SDL_Rect> Map::getColliders() {
    std::vector<SDL_Rect> colliders;

    for(int row = 0; row < mapData.size(); row++) {
        for(int col = 0; col < mapData[row].size(); col++) {
            int type = mapData[row][col];

            if (type == 1 || type == 2) {
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

std::vector<Collectible*> Map::generateHearts() {
    std::vector<Collectible*> generatedHearts;

    for(int row = 0; row < mapData.size(); row++) {
        for(int col = 0; col < mapData[row].size(); col++) {
            int type = mapData[row][col];

            if (type == 3) {
                int x = col * 32;
                int y = row * 32;

                Collectible* newHeart = new Collectible(x, y);
                generatedHearts.push_back(newHeart);

                mapData[row][col] = 0;
            }
        }
    }
    return generatedHearts;
}