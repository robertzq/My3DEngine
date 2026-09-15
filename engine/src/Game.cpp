#include "Engine/Game.h"
#include <sys/stat.h>
#include <cstdio>
#include "Engine/ResourceManager.h"
#include "Engine/TextRenderer.h"
#include "Engine/Renderer.h"
#include "Engine/PostProcess.h"
#include "Engine/MeshPageCurl.h"
#include "Engine/RedBorder.h"
#include "Engine/ShaderManager.h"
#include "Engine/Log.h"
#include "Engine/Time.h"
#include "Engine/Input.h"
#include "Engine/Audio.h"
#include "Engine/UI/UIManager.h"

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
        PostProcess::Init();
        MeshPageCurl::Init();
        RedBorder::Init();

        isRunning = true;
        Time::Reset();
        Audio::Init();
        UIManager::Init();
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
    // UI 先于 gameplay 处理输入：菜单打开时会切换 Input 上下文，屏蔽 gameplay 动作
    UIManager::HandleInput(Time::DeltaTime());
    UIManager::DispatchActions();   // 注册了 handler 时消费 action（菜单可在此 Push/Pop）
    // pause 语义由菜单定义决定，不等同于 UIManager::Active()
    if (!UIManager::ShouldPauseGameplay()) sceneManager.Update();
    UIManager::Update(Time::DeltaTime());
    if (!UIManager::ShouldPauseAll()) RedBorder::Update(Time::DeltaTime());
}

void Game::render() {
    int w = 0, h = 0;
    SDL_GL_GetDrawableSize(window, &w, &h);
    Renderer::BeginFrame(w, h);

    // World -> World FBO -> World PostFX -> composite -> (UI) -> Final PostFX -> screen
    PostProcess::Resize(w, h);
    PostProcess::BeginWorld();
    sceneManager.Render();
    PostProcess::EndWorld();

    if (sceneManager.InTransition()) {
        // 转场：old snapshot + new world -> PageCurl -> composite（替代 world FX 阶段）
        PostProcess::BindComposite();
        sceneManager.RenderTransition(w, h);
    } else {
        PostProcess::ApplyWorld();
    }

    // UI 绘制进 composite（World -> WorldFX -> UI），FinalFX（如 RedBorder）覆盖 UI
    PostProcess::BindComposite();
    UIManager::Render();

    PostProcess::ApplyFinal();

    Renderer::EndFrame();
}

void Game::clean() {
    UIManager::Clean();
    Audio::Clean();
    TextRenderer::Clean();        // 释放文字 GL 纹理（context 仍有效）
    ResourceManager::Clear();     // 释放纹理 GL 对象
    PostProcess::Clean();         // 释放 FBO / ping-pong / blit shader
    ShaderManager::Clean();       // 释放所有 shader（含默认 sprite shader）
    Renderer::Clean();            // 删除 quad / white / GL context
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
    LOG_INFO("引擎清理完成");
}
