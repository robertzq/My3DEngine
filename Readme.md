# MyEngine

一个轻量的 **C++17 + SDL2** 2D 游戏引擎库。核心只依赖 SDL2 / SDL2_image / SDL2_ttf，编译为静态库 `libengine.a`，游戏项目链接它即可复用。

引擎本身不包含任何游戏逻辑、美术资源或关卡数据 —— 你只需写自己的 `game/` 代码，`#include "Engine/..."` 即可。

---

## ✨ 特性

- **数据驱动场景**：`SceneManager` 从 JSON 配置读取场景定义，负责地图加载、出生点、图块触发器与场景切换；`SceneView`（只渲染）与 `SceneController`（逻辑）分离，符合 MVC。
- **组件化实体**：`Entity`（Transform / Sprite / Collider）+ `Behavior`（按注册名挂载的逻辑），所有物体共用一套实体模型，运行时 `Spawn` / `Destroy`。
- **Y 轴深度排序**：地图分 `ground` / `overlay` 两层，`overlay` 图块与实体按脚底 Y 统一排序绘制，实现“走到树/房子后面被遮挡”。
- **通用瓦片地图**：`TileSet` / `TileMap` 支持空格或紧凑格式 `.map`，图块的纹理、阻挡、触发器均由配置决定，不含任何硬编码图块。
- **场景管理**：基于 `Scene` 基类的状态管理，`SceneFactory` 支持场景自注册与工厂创建（兼容旧用法）。
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
│   ├── Game.h              #   游戏循环 + SDL 初始化（持有 SceneManager）
│   ├── SceneManager.h      #   控制器：配置加载 / 场景切换 / 相机 / 触发器
│   ├── SceneData.h         #   数据模型：场景定义 / 转场 / 出生点
│   ├── TileSet.h           #   图块集：纹理 / 阻挡 / 触发器名
│   ├── TileMap.h           #   通用瓦片地图：解析 / 绘制 / 碰撞盒 / 触发器
│   ├── SceneView.h         #   视图基类（场景脚本，只负责渲染）
│   ├── SceneController.h   #   控制器基类（游戏逻辑）
│   ├── SceneContext.h      #   传给 View / Controller 的运行时上下文
│   ├── SceneRegistry.h     #   View / Controller 自注册工厂
│   ├── Entity.h            #   通用实体：Transform / Sprite / Collider
│   ├── Behavior.h          #   行为基类（实体逻辑）
│   ├── BehaviorRegistry.h  #   行为自注册工厂
│   ├── Scene.h             #   旧式场景基类（兼容）
│   ├── SceneFactory.h      #   旧式场景自注册工厂（兼容）
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

常用静态成员：`Game::renderer`（SDL 渲染器）、`Game::camera`（摄像机矩形）、`Game::event`（当前事件）、`Game::instance()`（单例）、`game.scenes()`（数据驱动的 `SceneManager`）。

> `Game` 的 `update/render/handleEvents` 会把调用委托给 `game.scenes()`（若已 `Start()`），
> 否则回退到旧的 `currentScene`。

### 2. `Scene` —— 场景基类（兼容旧用法）

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

> `Scene` / `SceneFactory` 为兼容旧项目的写法。新项目推荐下面数据驱动的
> `SceneManager` + `SceneView` / `SceneController`。

### 6. `TileSet` / `TileMap` —— 通用瓦片地图

图块集描述「每个数字对应什么纹理、是否阻挡、触发哪个事件」，全部由数据决定，
引擎里没有任何游戏专属图块：

```cpp
#include "Engine/TileMap.h"

TileSet tileSet;
tileSet.tileSize = 32;
tileSet.tiles[0] = { "grass.png" };                 // texture / solid / trigger
tileSet.tiles[1] = { "tree.png", true };            // 阻挡
tileSet.tiles[7] = { "door.png", false, "door" };   // 触发器

TileMap map;
map.Load("village.map", tileSet);
map.Draw(Game::renderer, Game::camera);

const std::vector<SDL_Rect>& solid = map.Colliders();   // 所有阻挡格
std::vector<SDL_Rect> doors = map.TriggerRects("door"); // 所有同名触发器
```

`.map` 同时支持空格分隔（`1 2 3`）与紧凑数字格式（`123`）。

### 7. 数据驱动场景（`SceneManager` + 配置文件）

`SceneManager` 是引擎侧控制器：读配置、加载地图、按出生点放置玩家、
处理图块触发器转场、跟随相机。游戏只需提供 `config.json`。

```jsonc
{
  "initial": { "scene": "village", "spawn": "default" },
  "tilesets": {
    "overworld": {
      "tile_size": 32,
      "tiles": {
        "1": { "texture": "tree.png", "solid": true },
        "7": { "texture": "door.png", "trigger": "to_house1" },
        "15": { "texture": "entrance.png", "trigger": "exit" },
        "17": { "texture": "mic.png", "trigger": "boss" }
      }
    }
  },
  "scenes": {
    "village": {
      "view": "VillageView",              // SceneRegistry 注册名
      "controller": "VillageController",  // 可选
      "map": "village.map",
      "tileset": "overworld",
      "spawns": { "default": [100, 100], "from_house1": [192, 230] },
      "player": {
        "behavior": "Player",           // BehaviorRegistry 注册名
        "tag": "player",
        "texture": "player.png",
        "size": [50, 50],
        "params": { "frames": 4, "rows": 4, "speed": 4 }
      },
      "transitions": [
        { "trigger": "to_house1", "target": "house1", "spawn": "entrance" }
      ]
    },
    "battle": {
      "view": "BattleView",
      "controller": "BattleController",
      "params": { "return_scene": "village" }
    }
  }
}
```

```cpp
Game game;
game.init("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
          EngineConfig::SCREEN_WIDTH, EngineConfig::SCREEN_HEIGHT, false);

game.scenes().LoadConfig("config.json");
game.scenes().Start();                 // 从 config 的 initial 场景开始

game.update();                          // 内部：controller.Update -> 触发器检查 -> 相机跟随
```

* 踩到 `trigger` 图块时，引擎按 `transitions` 自动切到 `target` 场景的 `spawn` 出生点。
* `automatic: false` 的转场不会被自动处理，交给游戏 Controller：
  `context.manager->RequestTransition("boss", { {"spawn_x", 100}, {"spawn_y", 200} });`
* `RequestScene(target, spawn, params)` 可携带运行时参数；`spawn_x` / `spawn_y` 会覆盖配置出生点。
* `player` 会按 `spawns` 生成并打上 `tag`（供相机/触发器使用）；`entities` 数组可放任意静态实体。

### 8. `SceneView` / `SceneController` —— MVC 场景脚本

视图只负责渲染，逻辑放在控制器；两者用注册名与配置里的 `view` / `controller` 对应：

```cpp
#include "Engine/SceneController.h"
#include "Engine/SceneView.h"
#include "Engine/SceneRegistry.h"

class VillageController : public SceneController {
public:
    void OnEnter(SceneContext& ctx) override {
        SDL_Point p = ctx.manager->SpawnPosition(ctx.params.value("spawn", "default"), ctx.params);
        player = new GameObject("player.png", Game::renderer, p.x, p.y, 1);
        ctx.manager->SetPlayer(player);   // 引擎自动设置世界边界
    }
    void OnExit() override { delete player; }
    void Update(SceneContext& ctx) override {
        player->Update();
        Physics::ResolveRPGCollision(player, ctx.map->Colliders());
    }
private:
    GameObject* player = nullptr;
};

class VillageView : public SceneView {
public:
    void OnEnter(SceneContext& ctx) override { ctrl = static_cast<VillageController*>(ctx.controller); }
    void Render(SceneContext& ctx) override {
        ctx.map->Draw(Game::renderer, Game::camera);
        ctrl->player->Render();
    }
private:
    VillageController* ctrl = nullptr;
};

static SceneRegistry::ControllerProxy c("VillageController", [] { return new VillageController(); });
static SceneRegistry::ViewProxy v("VillageView", [] { return new VillageView(); });
```

`SceneContext` 提供 `game / manager / controller / map / data / params`。

### 9. `Entity` / `Behavior` —— 组件化实体

所有物体都是同一个 `Entity`（`Transform` + `Sprite` + `Collider` + `tag`），
行为挂在实体上，引擎用 `BehaviorRegistry` 按注册名创建：

```cpp
#include "Engine/Entity.h"
#include "Engine/Behavior.h"
#include "Engine/BehaviorRegistry.h"

class CollectibleBehavior : public Behavior {
public:
    void OnSpawn(SceneContext& ctx) override {
        self->sprite.texture = ResourceManager::GetTexture("heart.png");
        self->transform.w = 40;
        self->transform.h = 32;
        self->collider.offset = {0, 0, 40, 32};   // 碰撞盒
        self->collider.enabled = true;
    }
    void Update(SceneContext& ctx, float dt) override { /* 每帧逻辑 */ }
    bool collected = false;
};

static BehaviorRegistry::Proxy proxy("Collectible", [] { return new CollectibleBehavior(); });
```

`SceneManager` 负责实体生命周期与统一流程：

```cpp
Entity* e = ctx.manager->Spawn("GiftBox", "gift", 320, 288);  // 生成并执行 OnSpawn
ctx.manager->Destroy(e);                                       // 延迟回收
for (Entity* e : ctx.manager->Entities()) { /* ... */ }
Entity* player = ctx.manager->Player();                        // tag == "player"
```

每帧顺序：更新所有 `Behavior` → 跑场景 `SceneController` → 回收死亡实体 → 检查图块触发器 → 相机跟随。
`SceneView` 里只需 `ctx.manager->DrawWorld()`，再叠加自己的 HUD / 过场。

碰撞直接作用于 `Entity`：`Physics::MoveTopDown(entity, vx, vy, obstacles)`（俯视）、
`Physics::MovePlatformer(entity, vx, vy, onGround, obstacles)`（横版）。

### 10. Y 轴深度排序（遮挡）

按物体“脚底”的世界 Y 排序：Y 小的先画（在后面），Y 大的后画（在前面）。

* 地图在 tileset 里用 `"overlay": true` 标记参与排序的图块（树、房子、门、栅栏），
  其余为 `ground` 背景；`TileMap::Overlays()` 暴露它们，排序键 = 图块底部 Y。
* 实体排序键 = `Entity::SortKey()`（默认 `transform.y + transform.h`，可用 `sortYOffset` 微调）。
* `SceneManager::DrawWorld()` 先画 ground，再把 overlay 图块与所有可见实体合并成
  一个列表做稳定排序后绘制。

```jsonc
"3": { "texture": "tree.png", "solid": true, "overlay": true },
"4": { "texture": "wall.png", "solid": true, "overlay": true },
"5": { "texture": "roof.png", "solid": true, "overlay": true }
```

这样玩家走到树/房子的北面会被挡住，走到南面则盖住它们。

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
