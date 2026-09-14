#include "Engine/Game.h"
#include "Engine/ResourceManager.h"
#include "Engine/Log.h"
#include "Engine/Time.h"
#include "Engine/Input.h"
#include "Engine/Audio.h"

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

        Time::Reset();
        Audio::Init();
    } else {
        isRunning = false;
    }
}

void Game::handleEvents() {
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                isRunning = false;
                break;
            default:
                sceneManager.HandleEvent(event);
                break;
        }
    }
}

void Game::update() {
    Input::Update();
    Time::Tick();
    sceneManager.Update();
}

void Game::render() {
    SDL_RenderClear(renderer);
    sceneManager.Render();
    SDL_RenderPresent(renderer);
}

void Game::clean() {
    Audio::Clean();
    ResourceManager::Clear();
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
    LOG_INFO("引擎清理完成");
}
