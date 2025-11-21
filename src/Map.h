#pragma once
#include "Game.h"
#include <string> // 新增
#include <fstream> // 新增
// 假设我们要个宽一点的图
#define MAP_WIDTH 100
#define MAP_HEIGHT 20
class Collectible;
class Map {
public:
    Map();
    ~Map();

   // 修改：不再传数组，而是传文件路径
    void LoadMap(std::string path);
    void DrawMap();

    // 【新增】获取所有的障碍物矩形
    std::vector<SDL_Rect> getColliders();
    // 【新增】这是一个工厂函数，它扫描地图，把所有的 '3' 变成爱心对象返回
    std::vector<Collectible*> generateHearts();

private:
    SDL_Rect srcRect, destRect;
    
    // 存储三种地形的纹理
    SDL_Texture* dirt;
    SDL_Texture* grass;
    SDL_Texture* water;
    SDL_Texture* heart;

    // 地图数组
    int map[MAP_HEIGHT][MAP_WIDTH];
};