# MyEngine

一个轻量的 **C++17 + SDL2** 2D 游戏引擎库。核心只依赖 SDL2 / SDL2_image / SDL2_ttf，编译为静态库 `libengine.a`，游戏项目链接它即可复用。

引擎本身不包含任何游戏逻辑、美术资源或关卡数据 —— 你只需写自己的 `game/` 代码，`#include "Engine/..."` 即可。

---

## ✨ 特性

- **场景管理**：基于 `Scene` 基类的状态管理，`SceneFactory` 支持场景自注册与工厂创建。
- **游戏循环**：`Game` 负责 SDL 初始化、事件分发、更新与渲染，单例访问 `Game::instance()`。
- **资源管理**：`ResourceManager` 从内存资源表加载并缓存纹理 / 文本，支持资源表注入。
- **游戏对象**：`GameObject` 提供渲染、帧动画、重力、速度与碰撞盒。
- **物理碰撞**：AABB 碰撞检测，横向卷轴与俯视角两套碰撞解析。
- **文字渲染**：`TextRenderer` 基于 SDL_ttf，带纹理缓存与抗锯齿。
- **输入**：`Input` 键盘状态轮询，避免按键状态残留。
- **日志**：`Log` 分级日志宏 `LOG_DEBUG/INFO/WARN/ERROR`。

---

## 📂 目录结构

```text
engine/
├── CMakeLists.txt          # 引擎库构建配置（含 SDL 平台检测）
├── include/Engine/         # 公共 API 头文件
│   ├── Game.h              #   游戏循环 + 场景管理
│   ├── Scene.h             #   场景基类
│   ├── SceneFactory.h      #   场景自注册工厂
│   ├── GameObject.h        #   通用游戏对象基类
│   ├── ResourceManager.h   #   资源加载 / 缓存 / 绘制
│   ├── TextRenderer.h      #   文字渲染（SDL_ttf）
│   ├── Input.h             #   输入轮询
│   ├── Physics.h           #   AABB 碰撞检测
│   ├── Config.h            #   引擎常量
│   ├── Log.h               #   分级日志
│   └── json.hpp            #   nlohmann/json（第三方）
└── src/                    # 引擎实现
```

---

## 🚀 环境依赖

### macOS (Homebrew)

```shell
brew install cmake
brew install sdl2
brew install sdl2_image
brew install sdl2_ttf
```

### Windows

推荐使用 vcpkg 安装 SDL2 系列库，或将开发包放到 `dependencies/` 目录。CMake 已包含 MSVC 的 UTF-8 编码修复与路径检测。

---

## 🛠 集成到你的项目

有两种方式：

### 方式 A：`add_subdirectory`（推荐）

把你的游戏目录放进本仓库（或把 `engine/` 作为子目录），在顶层 `CMakeLists.txt` 中：

```cmake
cmake_minimum_required(VERSION 3.14)
project(MyGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(engine)   # 引擎库，target 名为 engine

add_executable(MyGame
    src/main.cpp
    src/MyScene.cpp
)
target_include_directories(MyGame PRIVATE src)
target_link_libraries(MyGame PRIVATE engine)
```

`engine` 的公共头文件路径 `engine/include` 会通过 `target_include_directories(... PUBLIC ...)` 自动传递给 `MyGame`，因此直接 `#include "Engine/Game.h"` 即可。

### 方式 B：先编译静态库

```shell
mkdir build && cd build
cmake ..
make
```

得到 `libengine.a`，之后在自己的项目中手动链接它和 SDL 三个库。

---

## 📖 核心概念

### 1. `Game` —— 游戏循环

引擎只负责「循环 + 场景切换」，不含任何游戏逻辑：

```cpp
#include "Engine/Game.h"
#include "Engine/Config.h"

Game game;
game.init("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
          EngineConfig::SCREEN_WIDTH, EngineConfig::SCREEN_HEIGHT, false);

while (game.running()) {
    game.handleEvents();  // 事件分发（转发给当前场景）
    game.update();        // 更新当前场景
    game.render();        // 渲染当前场景
    SDL_Delay(1000 / EngineConfig::TARGET_FPS);
}

game.clean();
```

常用静态成员：`Game::renderer`（SDL 渲染器）、`Game::camera`（摄像机矩形）、`Game::event`（当前事件）、`Game::instance()`（单例）。

### 2. `Scene` —— 场景基类

所有游戏场景继承 `Scene`，实现 5 个虚方法：

```cpp
#include "Engine/Scene.h"

class MyScene : public Scene {
public:
    void OnEnter() override {}                          // 进入：加载资源、创建对象
    void OnExit() override {}                           // 离开：清理内存
    void HandleEvents(SDL_Event& event) override {}     // 处理输入事件
    void Update() override {}                           // 每帧逻辑
    void Render() override {}                           // 每帧渲染
};
```

切换场景：

```cpp
game.ChangeScene(new MyScene());  // 引擎负责 delete 旧场景并调用新场景 OnEnter
```

### 3. `ResourceManager` —— 资源注入

引擎从**内存资源表**加载资源，表由游戏在启动时注入，引擎不关心资源来源：

```cpp
#include "Engine/ResourceManager.h"

// 游戏侧：注册自己的资源表
ResourceManager::SetResourceTable(EMBEDDED_ASSETS);
ResourceManager::Init();

// 之后即可通过 ID 获取
SDL_Texture* tex = ResourceManager::GetTexture("player.png");
std::string  map = ResourceManager::GetTextContent("level1.map");
```

> 资源表类型为 `ResourceTable = std::map<std::string, EmbeddedResource>`，
> 其中 `EmbeddedResource { const unsigned char* data; size_t size; }`。

### 4. `GameObject` —— 游戏对象基类

```cpp
#include "Engine/GameObject.h"

GameObject* obj = new GameObject("player.png", Game::renderer, 100, 100, 4);
obj->SetVelX(4);               // 水平速度
obj->SetGravityEnabled(false); // 俯视角关闭重力（默认开启）
obj->Update();                 // 推进物理 + 动画
obj->Render();                 // 绘制（含摄像机偏移）
SDL_Rect box = obj->GetBounds(); // 碰撞盒
```

### 5. `SceneFactory` —— 场景自注册

场景文件末尾用 `Proxy` 注册，之后可用字符串创建场景（便于数据驱动）：

```cpp
#include "Engine/SceneFactory.h"
#include "Engine/json.hpp"

static SceneFactory::Proxy proxy_MyScene("MyScene", [](const json& params) {
    return new MyScene();
});

// 使用：
Scene* s = SceneFactory::Create("MyScene", params);
```

---

## 🎯 最小可运行示例

下面是一个完整的最小游戏：窗口 + 一个 WASD 移动的方块。

```cpp
// main.cpp
#include "Engine/Game.h"
#include "Engine/Scene.h"
#include "Engine/GameObject.h"
#include "Engine/ResourceManager.h"
#include "Engine/Input.h"
#include "Engine/Config.h"

class PlayScene : public Scene {
    GameObject* player = nullptr;
public:
    void OnEnter() override {
        // 资源表中没有 "player.png" 时，GameObject 会绘制红色占位方块
        player = new GameObject("player.png", Game::renderer, 100, 100, 1);
        player->SetGravityEnabled(false);   // 俯视角移动
        player->SetWorldWidth(EngineConfig::SCREEN_WIDTH);
        player->SetWorldHeight(EngineConfig::SCREEN_HEIGHT);
    }
    void OnExit() override { delete player; }
    void HandleEvents(SDL_Event& e) override {}

    void Update() override {
        int dx = 0, dy = 0;
        if (Input::IsKeyDown(SDL_SCANCODE_A)) dx -= 4;
        if (Input::IsKeyDown(SDL_SCANCODE_D)) dx += 4;
        if (Input::IsKeyDown(SDL_SCANCODE_W)) dy -= 4;
        if (Input::IsKeyDown(SDL_SCANCODE_S)) dy += 4;
        player->SetVelX(dx);
        player->SetVelY(dy);
        player->Update();
    }
    void Render() override { player->Render(); }
};

int main() {
    ResourceManager::Init();
    Game game;
    game.init("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
              EngineConfig::SCREEN_WIDTH, EngineConfig::SCREEN_HEIGHT, false);
    game.ChangeScene(new PlayScene());

    const int frameDelay = 1000 / EngineConfig::TARGET_FPS;
    while (game.running()) {
        Uint32 start = SDL_GetTicks();
        game.handleEvents();
        game.update();
        game.render();
        Uint32 elapsed = SDL_GetTicks() - start;
        if (frameDelay > elapsed) SDL_Delay(frameDelay - elapsed);
    }
    game.clean();
    return 0;
}
```

---

## 📦 资源打包

引擎从 `EmbeddedResource` 内存表加载，你可以：

1. **手写小资源**（适合单个小文件）：

```cpp
#include "Engine/ResourceManager.h"

static const unsigned char my_png[] = { 0x89, 0x50, /* ... 完整字节 ... */ };

inline const ResourceTable EMBEDDED_ASSETS = {
    { "my.png", { my_png, sizeof(my_png) } },
};

// main 里：
ResourceManager::SetResourceTable(EMBEDDED_ASSETS);
```

2. **用脚本批量打包**（推荐，把 `assets/` 目录自动转成 `EmbeddedAssets.h`）：

写一个脚本遍历资源目录，为每个文件生成 `inline const unsigned char RES_xxx[]`，
再汇总成 `inline const ResourceTable EMBEDDED_ASSETS`，在 `main` 里 `SetResourceTable` 注入即可。
（可参考原游戏项目的 `game/tools/embed_assets.py` 实现。）

---

## 🛠 其他子系统速查

```cpp
// 文字渲染
#include "Engine/TextRenderer.h"
TextRenderer::Init("font.ttf", 24);                 // 字体资源 ID + 字号
TextRenderer::DrawText(10, 10, "Hello", {255,255,255,255});

// 碰撞检测
#include "Engine/Physics.h"
bool hit = Physics::CheckCollision(rectA, rectB);
Physics::ResolveRPGCollision(obj, obstacles);       // 俯视角（推挤回退）

// 日志
#include "Engine/Log.h"
LOG_INFO("资源数量: " << count);
LOG_ERROR("加载失败: " << id);
```

---

## 📄 License

引擎代码可自由使用。`engine/include/Engine/json.hpp` 为 [nlohmann/json](https://github.com/nlohmann/json)（MIT License）。
