// src/Map/Map.h
#pragma once
#include "Game.h"
#include <string>
#include <vector>
#include <iostream>

class Collectible;

class Map {
public:
    Map();
    ~Map();

    void LoadMap(std::string path);
    void DrawMap();

    // 获取障碍物（碰撞箱）
    std::vector<SDL_Rect> getColliders();

    // 获取指定类型的图块（用于寻找门、BOSS、出生点）
    std::vector<SDL_Rect> GetTiles(int tileID);

    // 【新增】获取地图尺寸
    int GetWidth() { return mapWidth; }
    int GetHeight() { return mapHeight; }
    std::vector<Collectible*> generateHearts();

private:
    SDL_Rect srcRect, destRect;

    // --- 纹理资源 ---
    // 通用/村庄
    SDL_Texture* tex_grass;     // 0
    SDL_Texture* tex_tree;      // 1
    SDL_Texture* tex_path;      // 2
    SDL_Texture* tex_water;     // 3
    SDL_Texture* tex_bridge;    // 4
    SDL_Texture* tex_wall;      // 5
    SDL_Texture* tex_roof;      // 6
    SDL_Texture* tex_door;      // 7
    SDL_Texture* tex_flower;    // 8
    SDL_Texture* tex_fence;     // 9

    // 室内
    SDL_Texture* tex_wood;      // 10
    SDL_Texture* tex_inWall;    // 11
    SDL_Texture* tex_rug;       // 12
    SDL_Texture* tex_sofa;      // 13
    SDL_Texture* tex_stage;     // 14
    SDL_Texture* tex_entrance;  // 15
    SDL_Texture* tex_decor;     // 16
    SDL_Texture* tex_mic;     // 17

    std::vector<std::vector<int>> mapData;
    int mapHeight = 0;
    int mapWidth = 0;
};