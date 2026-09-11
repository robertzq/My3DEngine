#include "Engine/Game.h"
#include "Engine/ResourceManager.h"
#include "Engine/TextRenderer.h"
#include "Engine/SceneFactory.h"
#include "Engine/Config.h"
#include "Engine/Log.h"
#include "Engine/json.hpp"
#include "EmbeddedAssets.h"
#include "GameState.h"
#include <vector>

using json = nlohmann::json;

// 关卡序列（从 config.json 加载，属于游戏侧，不属于引擎）
static std::vector<json> g_levelSequence;
static int g_currentLevelIndex = 0;

// 加载指定索引的关卡（原引擎 Game::LoadLevel 的逻辑，移到游戏侧）
static void LoadLevel(int index) {
    if (index < 0 || index >= (int)g_levelSequence.size()) {
        LOG_WARN("已经是最后一关，或者索引无效!");
        return;
    }

    json levelConfig = g_levelSequence[index];
    std::string type = levelConfig["type"];   // 例如 "VillageScene"
    json params = levelConfig["params"];      // 例如 { "map_file": "village.map" }

    LOG_INFO("正在加载关卡 " << index << ": " << levelConfig.value("name", "Unknown"));

    Scene* newScene = SceneFactory::Create(type, params);
    if (newScene) {
        Game::instance()->ChangeScene(newScene);
        g_currentLevelIndex = index;
    }
}

int main(int argc, char* argv[]) {
    const int FPS = EngineConfig::TARGET_FPS;
    const int frameDelay = 1000 / FPS;

    // 1. 注册游戏资源表到引擎
    ResourceManager::SetResourceTable(EMBEDDED_ASSETS);
    ResourceManager::Init();

    // 2. 初始化引擎
    Game game;
    game.init("RBT Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
              EngineConfig::SCREEN_WIDTH, EngineConfig::SCREEN_HEIGHT, false);

    // 3. 初始化字体（游戏特定资源）
    if (!TextRenderer::Init("fusion-pixel-12px-monospaced-zh_hans.ttf", 24)) {
        LOG_ERROR("字体初始化失败");
    }

    // 4. 读取关卡序列并加载初始关卡
    std::string configStr = ResourceManager::GetTextContent("config.json");
    auto config = json::parse(configStr);
    if (config.contains("level_sequence")) {
        g_levelSequence = config["level_sequence"].get<std::vector<json>>();
    }
    g_currentLevelIndex = config.value("initial_index", 0);
    LoadLevel(g_currentLevelIndex);

    // 5. 主循环
    Uint32 frameStart;
    int frameTime;
    while (game.running()) {
        frameStart = SDL_GetTicks();

        game.handleEvents();
        game.update();
        game.render();

        frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
    }

    game.clean();
    return 0;
}
