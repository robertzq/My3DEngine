// MyEngine 最小 demo：
// 直接使用引擎 Game 类创建窗口并运行主循环，验证引擎可链接、可运行，
// 同时验证 myengine_setup_target() 的 GUI 子系统（无控制台黑框）和 DLL 自动部署。
#include "Engine/Game.h"
#include "Engine/Log.h"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    Game game;

    game.init("MyEngine Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
              EngineConfig::SCREEN_WIDTH, EngineConfig::SCREEN_HEIGHT, false);
    if (!game.running()) {
        LOG_ERROR("Engine failed to initialize");
        return 1;
    }

    LOG_INFO("Engine demo running. Close window to exit.");

    // 运行主循环（约 120 帧后自动退出，便于无头验证与自动化测试）
    const int MAX_FRAMES = 120;
    int frames = 0;
    while (game.running() && frames < MAX_FRAMES) {
        game.handleEvents();
        game.update();
        game.render();
        SDL_Delay(16); // ~60fps
        ++frames;
    }

    game.clean();
    LOG_INFO("Engine demo finished cleanly.");
    return 0;
}