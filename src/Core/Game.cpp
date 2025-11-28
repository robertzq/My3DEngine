#include "Game.h"
#include "SceneFactory.h"
#include "ResourceManager.h"
#include "TextRenderer.h"

// 静态成员初始化
SDL_Renderer* Game::renderer = nullptr;
SDL_Event Game::event;
SDL_Rect Game::camera = {0, 0, 800, 600};
float Game::cameraX_float = 0.0f;

// === 【新增】初始化单例指针 ===
Game* Game::s_instance = nullptr;

Game::Game() {
    s_instance = this;
}
Game::~Game() {
// 可以在这里置空，虽然程序结束了也没关系
    s_instance = nullptr;
}

void Game::init(const char* title, int xpos, int ypos, int width, int height, bool fullscreen) {
    int flags = 0;
    if (fullscreen) flags = SDL_WINDOW_FULLSCREEN;

    if (SDL_Init(SDL_INIT_EVERYTHING) == 0) {
        std::cout << "SDL 初始化成功..." << std::endl;
        if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
             std::cout << "IMG_Init 失败: " << IMG_GetError() << std::endl;
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
    ResourceManager::Init();
   // === 【核心变化 1】只注册类型，不注册具体关卡 ===
    if (!TextRenderer::Init("msyh.ttf", 24)) { // 字体文件叫 NISC18030.ttf
            std::cout << "字体初始化失败" << std::endl;
    }
   // === 【核心变化 2】读取序列 ===
   std::string configStr = ResourceManager::GetTextContent("config.json");
   auto config = json::parse(configStr);

   // 保存关卡列表
   if (config.contains("level_sequence")) {
       levelSequence = config["level_sequence"].get<std::vector<json>>();
   }

   // 启动初始关卡
   currentLevelIndex = config.value("initial_index", 0);
   LoadLevel(currentLevelIndex);
}

// 切换场景逻辑
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
    SDL_PollEvent(&event);
    switch (event.type) {
        case SDL_QUIT:
            isRunning = false;
            break;
        default:
            // 把事件传给当前场景处理
            if (currentScene) currentScene->HandleEvents(event);
            break;
    }
}

void Game::update() {
    // 委托给当前场景
    if (currentScene) currentScene->Update();
}

void Game::render() {
    SDL_RenderClear(renderer);

    // 委托给当前场景
    if (currentScene) currentScene->Render();

    SDL_RenderPresent(renderer);
}

void Game::clean() {
    // 清理场景
    if (currentScene) {
        currentScene->OnExit();
        delete currentScene;
    }
    ResourceManager::Clean();
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
    std::cout << "引擎清理完成" << std::endl;
}

// 【新增】加载关卡的逻辑
void Game::LoadLevel(int index) {
    if (index < 0 || index >= levelSequence.size()) {
        std::cout << "已经是最后一关，或者索引无效!" << std::endl;
        return;
    }

    // 1. 获取该关卡的配置
    json levelConfig = levelSequence[index];
    std::string type = levelConfig["type"];     // 例如 "PlayScene"
    json params = levelConfig["params"];        // 例如 { "map_file": "level1.map" }

    std::cout << "正在加载关卡 " << index << ": " << levelConfig.value("name", "Unknown") << std::endl;

    // 2. 通过工厂创建
    Scene* newScene = SceneFactory::Create(type, params);

    // 3. 切换
    if (newScene) {
        ChangeScene(newScene);
        currentLevelIndex = index; // 更新索引
    }
}

void Game::NextLevel() {
    LoadLevel(currentLevelIndex + 1);
}