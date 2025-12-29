#pragma once
#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include "Scene.h" // 包含基类
#include "json.hpp" // 引入刚才下载的库
using json = nlohmann::json; // 方便使用
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
    // === 【新增】单例访问方法 ===
    static Game* instance() { return s_instance; }

    // 全局静态变量 (渲染器和摄像机)
    static SDL_Renderer* renderer;
    static SDL_Event event;
    static SDL_Rect camera;
    static float cameraX_float;

    // 【新增】切换场景的方法
    void ChangeScene(Scene* newScene);

    // 【新增】加载指定索引的关卡
    void LoadLevel(int index);

    // 【新增】下一关 (给 PlayScene 或是触发器调用的)
    void NextLevel();

    int bossDefeatedCount = 0; // 新增：记录打败BOSS的数量

    int lastVillageX = -1;
    int lastVillageY = -1;

private:
    bool isRunning;
    SDL_Window* window;

    // 【核心变化】不再持有 player/map，而是持有场景
    Scene* currentScene = nullptr;
    // 【新增】存储关卡序列
    std::vector<json> levelSequence;
    int currentLevelIndex = 0;

    // === 【新增】单例指针变量 ===
    static Game* s_instance;
};