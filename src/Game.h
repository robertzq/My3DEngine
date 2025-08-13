#pragma once
#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <vector>

// 前向声明，告诉编译器：哪怕你还没看到 GameObject 的细节，但你要知道有这么个类存在
class GameObject; 

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

private:
    bool isRunning;
    SDL_Window* window;
    SDL_Renderer* renderer;
    
    // 【关键修复点】这里必须声明 player 指针！
    GameObject* player; 
};