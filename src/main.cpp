#include "Engine/Engine.h"
#include "Game/DemoScene.h"
#include <cstdio>

int main() {
    std::fprintf(stderr, "[dbg] main enter\n");

    Engine eng;
    if (!eng.init(1280, 720, "My3DEngine")) {
        std::fprintf(stderr, "[dbg] Engine::init FAILED\n");
        return -1;
    }
    std::fprintf(stderr, "[dbg] Engine::init OK\n");

    DemoScene scene;
    scene.init(&eng);

    while (!eng.shouldClose()) {
        eng.frameBegin();
        scene.update();
        scene.render();
        eng.frameEnd();
    }

    scene.shutdown();
    eng.shutdown();
    return 0;
}
