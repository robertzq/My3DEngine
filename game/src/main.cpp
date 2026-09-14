#include "Engine/Config.h"
#include "Engine/Game.h"
#include "Engine/Log.h"
#include "Engine/ResourceManager.h"
#include "Engine/TextRenderer.h"
#include "Engine/Audio.h"
#include "Engine/json.hpp"
#include "Engine/UI/UIManager.h"
#include "Engine/UI/UIAction.h"
#include "EmbeddedAssets.h"

int main(int argc, char* argv[]) {
    const int frameDelay = 1000 / EngineConfig::TARGET_FPS;

    ResourceManager::SetResourceTable(EMBEDDED_ASSETS);
    ResourceManager::SetBasePath("game/assets");
    ResourceManager::Init();

    Game game;
    game.init("RBT Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
              EngineConfig::SCREEN_WIDTH, EngineConfig::SCREEN_HEIGHT, false);

    if (!TextRenderer::Init("fusion-pixel-12px-monospaced-zh_hans.ttf", 24)) {
        LOG_ERROR("字体初始化失败");
    }

    if (!game.scenes().LoadConfig("config.json")) {
        LOG_ERROR("场景配置加载失败");
        game.clean();
        return 1;
    }
    game.scenes().Start();

    // 数据驱动菜单：从同一 config.json 读取 menus 段
    {
        const std::string cfg = ResourceManager::GetText("config.json");
        nlohmann::json j = nlohmann::json::parse(cfg, nullptr, false);
        if (!j.is_discarded() && j.contains("menus")) {
            UIManager::LoadMenus(j["menus"]);
        }
        UIManager::SetActionHandler([](const UIAction& a) {
            if (a.action == "start")      UIManager::Pop();
            else if (a.action == "quit")  Game::instance()->Quit();
        });
        UIManager::Push("start");   // 开始菜单（pause=gameplay：打开时暂停 gameplay）
    }

    Audio::PlayMusic("forget_me_not.mp3");

    while (game.running()) {
        Uint32 frameStart = SDL_GetTicks();

        game.handleEvents();
        game.update();
        game.render();

        Uint32 frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < static_cast<Uint32>(frameDelay)) {
            SDL_Delay(frameDelay - frameTime);
        }
    }

    game.clean();
    return 0;
}
