#pragma once
#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include "Engine/Scene.h"
#include "Engine/Config.h"

// 引擎核心：负责 SDL 初始化、主循环、场景管理。
// 不包含任何游戏特定逻辑（关卡序列、游戏状态、字体等由游戏侧负责）。
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
    static Game* instance() { return s_instance; }

    // 全局静态变量（渲染器、事件、摄像机）
    static SDL_Renderer* renderer;
    static SDL_Event event;
    static SDL_Rect camera;
    static float cameraX_float;

    // 切换场景（引擎负责场景所有权与生命周期）
    void ChangeScene(Scene* newScene);

private:
    bool isRunning;
    SDL_Window* window;
    Scene* currentScene = nullptr;

    static Game* s_instance;
};
