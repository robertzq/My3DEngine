#include "Engine/Game.h"
#include "Engine/ResourceManager.h"
#include "Engine/TextRenderer.h"
#include "Engine/Renderer.h"
#include "Engine/Log.h"
#include "Engine/Time.h"
#include "Engine/Input.h"
#include "Engine/Audio.h"

// 静态成员初始化
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
    int flags = SDL_WINDOW_OPENGL;
    if (fullscreen) flags |= SDL_WINDOW_FULLSCREEN;

    if (SDL_Init(SDL_INIT_EVERYTHING) == 0) {
        LOG_INFO("SDL 初始化成功...");

        // GL context 属性必须在 SDL_Init 之后、创建窗口之前设置
        Renderer::SetAttributes();

        if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
            LOG_ERROR("IMG_Init 失败: " << IMG_GetError());
            isRunning = false;
            return;
        }

        window = SDL_CreateWindow(title, xpos, ypos, width, height, flags);
        if (!window) {
            LOG_ERROR("SDL_CreateWindow 失败: " << SDL_GetError());
            isRunning = false;
            return;
        }

        if (!Renderer::Init(window)) {
            LOG_ERROR("Renderer 初始化失败");
            isRunning = false;
            return;
        }

        isRunning = true;
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
    int w = 0, h = 0;
    SDL_GL_GetDrawableSize(window, &w, &h);
    Renderer::BeginFrame(w, h);
    sceneManager.Render();
    Renderer::EndFrame();
}

void Game::clean() {
    Audio::Clean();
    TextRenderer::Clean();        // 释放文字 GL 纹理（context 仍有效）
    ResourceManager::Clear();     // 释放纹理 GL 对象
    Renderer::Clean();            // 删除 shader / quad / GL context
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
    LOG_INFO("引擎清理完成");
}
