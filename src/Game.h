#pragma once
#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <vector>
#include "Collision.h"


class GiftBox;
class Map;
// 前向声明，告诉编译器：哪怕你还没看到 GameObject 的细节，但你要知道有这么个类存在
class GameObject;
class Collectible;

class Game {
public:
    Game();
    ~Game();

    void init(const char* title, int xpos, int ypos, int width, int height, bool fullscreen);
    
    void handleEvents();
    void update();
    void render();
    void clean();

    bool running() { return isRunning; }
    static SDL_Renderer* renderer;
    static SDL_Event event; // 如果你之前没加，最好也加上
    
    // 【新增】全局摄像机
    static SDL_Rect camera;
    static float cameraX_float;

private:
    bool isRunning;
    SDL_Window* window;
    SDL_Rect groundCollision; // 定义一个地面碰撞框
    
    
    // 【关键修复点】这里必须声明 player 指针！
    GameObject* player; 
    Map* map; // 声明地图指针
    GiftBox* giftBox; // 新增

    std::vector<Collectible*> hearts;
};