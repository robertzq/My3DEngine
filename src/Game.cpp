#include "Game.h"
#include "TextureManager.h"
#include "GameObject.h"  // 必须包含这个，因为我们要用 player 的 new 方法

Game::Game() {}
Game::~Game() {}

// Game.cpp

void Game::init(const char* title, int xpos, int ypos, int width, int height, bool fullscreen) {
    int flags = 0;
    if (fullscreen) {
        flags = SDL_WINDOW_FULLSCREEN;
    }

    // 1. 初始化 SDL 基础功能
    if (SDL_Init(SDL_INIT_EVERYTHING) == 0) {
        std::cout << "SDL 初始化成功..." << std::endl;

        // --- 【新增】2. 初始化 PNG 图片加载模块 ---
        // 这一步非常关键！没有它，LoadTexture 读 PNG 会直接失败
        if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
            std::cout << "SDL_image 初始化失败: " << IMG_GetError() << std::endl;
            isRunning = false;
            return;
        }
        // ---------------------------------------

        window = SDL_CreateWindow(title, xpos, ypos, width, height, flags);
        renderer = SDL_CreateRenderer(window, -1, 0);
        
        if (renderer) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            isRunning = true;
        }
    } else {
        isRunning = false;
    }

    // 创建对象
    player = new GameObject("./assets/redPlayer.png", renderer, 0, 0,6);
}

// 在 Game.cpp 的 handleEvents 函数里
void Game::handleEvents() {
    SDL_Event event;
    SDL_PollEvent(&event);

    switch (event.type) {
        case SDL_QUIT:
            isRunning = false;
            break;

        // --- 按下按键：给速度 ---
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
                case SDLK_SPACE:
                    player->Jump();
                    break;
                case SDLK_a: // 按 A，向左走
                    player->SetVelX(-5); 
                    break;
                case SDLK_d: // 按 D，向右走
                    player->SetVelX(5);
                    break;
                default:
                    break;
            }
            break;

        // --- 松开按键：速度归零 ---
        case SDL_KEYUP:
            switch (event.key.keysym.sym) {
                case SDLK_a: 
                    // 松开 A，停下
                    player->SetVelX(0);
                    break;
                case SDLK_d:
                    // 松开 D，停下
                    player->SetVelX(0);
                    break;
                default:
                    break;
            }
            break;
            
        default:
            break;
    }
}

void Game::update() {
    // 【关键修复点】调用 player 的更新
    player->Update();
}

void Game::render() {
    SDL_RenderClear(renderer);
    
    // 【关键修复点】调用 player 的渲染
    player->Render();
   // enemy->Render();
    SDL_RenderPresent(renderer);
}

void Game::clean() {
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
    std::cout << "游戏清理完成" << std::endl;
}