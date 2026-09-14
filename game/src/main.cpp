#include "Engine/Config.h"
#include "Engine/Audio.h"
#include "Engine/Game.h"
#include "Engine/Log.h"
#include "Engine/ResourceManager.h"
#include "Engine/TextRenderer.h"
#include "EmbeddedAssets.h"

int main(int argc, char* argv[]) {
    const int frameDelay = 1000 / EngineConfig::TARGET_FPS;

    ResourceManager::SetResourceTable(EMBEDDED_ASSETS);
    ResourceManager::SetBasePath("game/assets");   // File 来源：大型资源（如 BGM）走磁盘
    ResourceManager::Init();

    Game game;
    game.init("Memory Island", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
              EngineConfig::SCREEN_WIDTH, EngineConfig::SCREEN_HEIGHT, false);

    TextRenderer::Init("fusion-pixel-12px-monospaced-zh_hans.ttf", 24);

    if (!game.scenes().LoadConfig("config.json")) {
        LOG_ERROR("场景配置加载失败");
        game.clean();
        return 1;
    }
    game.scenes().Start();

    Audio::PlayMusic("forget_me_not.mp3");   // 启动 BGM（经 File 来源，Scene 切换不中断）

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
