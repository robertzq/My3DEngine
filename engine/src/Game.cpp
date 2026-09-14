#include "Engine/Game.h"
#include "Engine/ResourceManager.h"
#include "Engine/Log.h"

// 静态成员初始化
SDL_Renderer* Game::renderer = nullptr;
SDL_Event Game::event;
SDL_Rect Game::camera = {0, 0, EngineConfig::SCREEN_WIDTH, EngineConfig::SCREEN_HEIGHT};
float Game::cameraX_float = 0.0f;

Game* Game::s_instance = nullptr;

Game::Game() {
    s_instance = this;
}

Game::~Game() {
    s_instance = nullptr;
}

void Game::init(const char* title, int xpos, int ypos, int width, int height, bool fullscreen) {
    int flags = 0;
    if (fullscreen) flags = SDL_WINDOW_FULLSCREEN;

    if (SDL_Init(SDL_INIT_EVERYTHING) == 0) {
        LOG_INFO("SDL 初始化成功...");
        if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
            LOG_ERROR("IMG_Init 失败: " << IMG_GetError());
            isRunning = false;
            return;
        }

        window = SDL_CreateWindow(title, xpos, ypos, width, height, flags);
        renderer = SDL_CreateRenderer(window, -1, 0);

        if (renderer) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            isRunning = true;
        }
    } else {
        isRunning = false;
    }
}

void Game::ChangeScene(Scene* newScene) {
    // 1. 如果当前有场景，先清理退出
    if (currentScene != nullptr) {
        currentScene->OnExit();
        delete currentScene;
    }

    // 2. 切换并进入新场景
    currentScene = newScene;
    if (currentScene != nullptr) {
        currentScene->OnEnter();
    }
}

void Game::handleEvents() {
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                isRunning = false;
                break;
            default:
                // 优先交给数据驱动的场景管理器，其次才是旧式 Scene
                if (sceneManager.Active()) sceneManager.HandleEvent(event);
                else if (currentScene) currentScene->HandleEvents(event);
                break;
        }
    }
}

void Game::update() {
    if (sceneManager.Active()) sceneManager.Update();
    else if (currentScene) currentScene->Update();
}

void Game::render() {
    SDL_RenderClear(renderer);
    if (sceneManager.Active()) sceneManager.Render();
    else if (currentScene) currentScene->Render();
    SDL_RenderPresent(renderer);
}

void Game::clean() {
    if (currentScene) {
        currentScene->OnExit();
        delete currentScene;
    }
    ResourceManager::Clean();
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
    LOG_INFO("引擎清理完成");
}
