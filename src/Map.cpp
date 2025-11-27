#include "Map.h"
#include "TextureManager.h"
#include "Collectible.h"
Map::Map() {
    // 加载地形素材
    // 请确保你在 assets 里放了这三张图，或者用一张 tileset
    // 这里为了演示方便，假设你有这三张图
    dirt = TextureManager::LoadTexture("./assets/dirt.png", Game::renderer);
    grass = TextureManager::LoadTexture("./assets/grass.png", Game::renderer);
    water = TextureManager::LoadTexture("./assets/water.png", Game::renderer);

    // 初始化渲染区域
    srcRect.x = srcRect.y = 0;
    srcRect.w = destRect.w = 32; // 假设格子是 32x32
    srcRect.h = destRect.h = 32;

    destRect.x = destRect.y = 0;
}

Map::~Map() {
    // 别忘了清理内存（虽然这只是个简单的演示）
    SDL_DestroyTexture(dirt);
    SDL_DestroyTexture(grass);
    SDL_DestroyTexture(water);
}

// 修改 LoadMap 函数
void Map::LoadMap(std::string path) {
    char c; // 用来存读取到的每一个字符
    std::fstream mapFile;

    mapFile.open(path);

    if (!mapFile.is_open()) {
        std::cout << "无法加载地图文件: " << path << std::endl;
        return;
    }

    int row = 0;
    int col = 0;

    // 循环读取文件里的每一个字符
    while (mapFile.get(c)) {
        // 如果是换行符，就跳到下一行
        if (c == '\n') {
            row++;
            col = 0;
            continue;
        }

        // 如果读到了数字字符 (比如 '0', '1', '2')
        if (c >= '0' && c <= '9') {
            // 这里有个小技巧：字符 '0' 的 ASCII 码是 48
            // 所以 '1' - '0' 就会得到整数 1
            if (row < 20 && col < 100) {
                map[row][col] = c - '0';
                col++;
            }
        }
    }

    mapFile.close();
    std::cout << "地图加载成功!" << std::endl;
}

void Map::DrawMap() {
    int type = 0;

    for(int row = 0; row < 20; row++) {
        for(int col = 0; col < 100; col++) {
            type = map[row][col]; // 获取当前格子的类型

            // 设置在屏幕上的位置
            destRect.x = col * 32; // 列数 * 格子宽
            destRect.y = row * 32; // 行数 * 格子高
            // 【关键】增加这一段视口剔除 (Culling) 优化
            // 如果这个格子在摄像机外面，就别画了，省资源！
            // 简单判断：位置 < camera.x - 32 或者 > camera.x + 800

            // --- 减去摄像机偏移 ---
            // 这里我们直接修改 destRect 的值用于绘制，
            // 但因为每次循环都会重新计算 destRect.x/y，所以是安全的。
            destRect.x = destRect.x - Game::camera.x;
            destRect.y = destRect.y - Game::camera.y;
            // 根据类型画不同的图
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

// 【新增】实现 getColliders
std::vector<SDL_Rect> Map::getColliders() {
    std::vector<SDL_Rect> colliders;

    for(int row = 0; row < 20; row++) {
        for(int col = 0; col < 100; col++) {
            int type = map[row][col];

            // 假设 1 是草，2 是土，它们都是障碍物
            // 0 是水/空气，不算障碍物
            if (type == 1 || type == 2) {
                SDL_Rect collider;
                collider.x = col * 32;
                collider.y = row * 32;
                collider.w = 32;
                collider.h = 32;
                
                // 把这个方块加入列表
                colliders.push_back(collider);
            }
        }
    }
    
    return colliders;
}

std::vector<Collectible*> Map::generateHearts() {
    std::vector<Collectible*> generatedHearts;

    for(int row = 0; row < 20; row++) {
        for(int col = 0; col < 100; col++) {
            int type = map[row][col];

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
                map[row][col] = 0;
            }
        }
    }

    return generatedHearts;
}