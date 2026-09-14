#pragma once
#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include "Engine/Config.h"
#include "Engine/SceneManager.h"

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

    // 全局静态变量（事件、摄像机）
    static SDL_Event event;
    static SDL_Rect camera;
    static float cameraX_float;

    // 数据驱动的场景管理器（引擎侧负责地图加载、转场、相机）
    SceneManager& scenes() { return sceneManager; }

private:
    bool isRunning;
    SDL_Window* window;
    SceneManager sceneManager;

    static Game* s_instance;
};
