# MyEngine

一个轻量的 **C++17 + SDL2** 2D 游戏引擎库。核心只依赖 SDL2 / SDL2_image / SDL2_ttf，编译为静态库 `libengine.a`，游戏项目链接它即可复用。

引擎本身不包含任何游戏逻辑、美术资源或关卡数据 —— 你只需写自己的 `game/` 代码，`#include "Engine/..."` 即可。

引擎侧统一采用 **数据驱动场景 + 组件化实体**：

```text
Model       (引擎)   SceneData / TileSet / TileMap / Entity(Transform/Sprite/Collider)  ← config.json 解析
Controller  (引擎)   SceneManager    ← 地图加载、出生点、触发器、转场、相机、实体生命周期
Controller  (游戏)   Behavior 子类    ← 实体逻辑（玩家移动、可拾取物等）
Controller  (游戏)   SceneController 子类（可选）← 场景级逻辑
View        (游戏)   SceneView 子类   ← 场景脚本，只负责每帧渲染
```

---

## ✨ 特性

- **数据驱动场景**：`SceneManager` 从 JSON 配置读取场景定义，负责地图加载、出生点、图块触发器与场景切换；`SceneView`（只渲染）与 `SceneController`（逻辑）分离，符合 MVC。
- **组件化实体**：`Entity`（Transform / Sprite / Collider / Animator）+ `Behavior`（按注册名挂载的逻辑），所有物体共用一套实体模型，运行时 `Spawn` / `Destroy`。
- **帧动画**：`Entity` 持有 `Animator`，按 `frames / fps / loop` 数据驱动播放，支持 `Play` / `Stop` / 循环 / `FlipX` / `FlipY`，计时使用 `Time::DeltaTime()`。
- **实体 / 视图 / 控制器自注册**：`BehaviorRegistry`、`SceneRegistry` 用 `Proxy` 按注册名创建，配置里用字符串引用。
- **Y 轴深度排序**：地图分 `ground` / `overlay` 两层，`overlay` 图块与实体按脚底 Y 统一排序绘制。
- **通用瓦片地图**：`TileSet` / `TileMap` 支持空格或紧凑格式 `.map`，图块的纹理、阻挡、overlay、触发器均由配置决定。
- **游戏循环**：`Game` 负责 SDL 初始化、事件分发、更新与渲染，单例访问 `Game::instance()`。
- **统一帧时间**：`Time` 提供每帧 `DeltaTime` / `UnscaledDeltaTime` / `ElapsedTime` / `FrameCount` 与 `TimeScale`，对异常大 dt 做 clamp。
- **资源管理**：`ResourceManager` 从内存资源表加载并缓存纹理 / 文本，支持资源表注入。
- **物理碰撞**：AABB 查询 / 解析分离，`Collider` 带 `layer`/`mask`/`isTrigger`，支持实体触发，横版与俯视角两套移动解析。
- **文字渲染**：`TextRenderer` 基于 SDL_ttf，带纹理缓存与抗锯齿。
- **输入**：`Input` 键盘状态轮询 + action 映射（`Down`/`Pressed`/`Released`/`Axis`），gameplay 不直接依赖 `SDL_SCANCODE`。
- **日志**：`Log` 分级日志宏 `LOG_DEBUG/INFO/WARN/ERROR`。

---

## 📂 目录结构

```text
engine/
├── CMakeLists.txt          # 引擎库构建配置（含 SDL 平台检测）
├── include/Engine/         # 公共 API 头文件
│   ├── Game.h              #   游戏循环 + SDL 初始化（持有 SceneManager）
│   ├── SceneManager.h      #   控制器：配置加载 / 场景切换 / 实体生命周期 / 相机 / 触发器
│   ├── SceneData.h         #   数据模型：场景定义 / 转场 / 出生点 / 实体定义
│   ├── TileSet.h           #   图块集：纹理 / 阻挡 / overlay / 触发器名
│   ├── TileMap.h           #   通用瓦片地图：解析 / 绘制 / 碰撞盒 / 触发器 / overlay
│   ├── SceneView.h         #   视图基类（场景脚本，只负责渲染）
│   ├── SceneController.h   #   场景级逻辑基类（可选）
│   ├── SceneContext.h      #   传给 View / Controller / Behavior 的运行时上下文
│   ├── SceneRegistry.h     #   View / Controller 自注册工厂
│   ├── Entity.h            #   通用实体：Transform / Sprite / Collider
│   ├── Behavior.h          #   行为基类（实体逻辑）
│   ├── BehaviorRegistry.h  #   行为自注册工厂
│   ├── ResourceManager.h   #   资源加载 / 缓存 / 绘制
│   ├── TextRenderer.h      #   文字渲染（SDL_ttf）
│   ├── Input.h             #   键盘轮询 + action 映射
│   ├── Physics.h           #   AABB 碰撞检测与移动解析
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
    src/PlayerBehavior.cpp
    src/WorldView.cpp
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

引擎只负责「循环 + SDL 初始化」，不含任何游戏逻辑：

```cpp
#include "Engine/Game.h"
#include "Engine/Config.h"

Game game;
game.init("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
          EngineConfig::SCREEN_WIDTH, EngineConfig::SCREEN_HEIGHT, false);

while (game.running()) {
    game.handleEvents();  // 分发 SDL 事件给 SceneManager
    game.update();        // 更新当前场景
    game.render();        // 渲染当前场景
    SDL_Delay(1000 / EngineConfig::TARGET_FPS);
}

game.clean();
```

常用成员：`Game::renderer`（SDL 渲染器）、`Game::camera`（摄像机矩形）、`Game::event`（当前事件）、`Game::instance()`（单例）、`game.scenes()`（数据驱动的 `SceneManager`）。

`game.update()` 内部先推进帧时钟再更新场景；`SceneManager` 会把本帧 `DeltaTime` 传给每个 `Behavior::Update(ctx, dt)`：

```cpp
#include "Engine/Time.h"

float dt = Time::DeltaTime();          // 本帧时间，受 TimeScale 影响（移动/动画用）
float raw = Time::UnscaledDeltaTime(); // 真实帧时间（UI / 暂停菜单用）
double t  = Time::ElapsedTime();       // 累计游戏时间（秒）
Time::SetTimeScale(0.0f);              // 暂停；恢复用 1.0f
```

移动速度使用「像素/秒 × dt」表达，保证不同帧率下速度一致。

`handleEvents` / `update` / `render` 全部委托给 `game.scenes()`；场景尚未 `Start()` 时 `SceneManager::Update/Render` 会直接返回。

### 2. 配置文件与 `SceneData`

场景、图块集、出生点、转场全部写在 `config.json` 里，引擎启动时解析为 `SceneData`：

```jsonc
{
  "initial": { "scene": "village", "spawn": "default" },

  "tilesets": {
    "overworld": {
      "tile_size": 32,
      "tiles": {
        "0": { "texture": "grass.png" },
        "1": { "texture": "tree.png", "solid": true, "overlay": true },
        "7": { "texture": "door.png", "trigger": "to_house1" }
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
        "texture": "rpgPlayer.png",
        "size": [50, 50],
        "params": { "speed": 4 }
      },
      "entities": [
        { "id": "sign", "behavior": "Sign", "tag": "npc", "texture": "sign.png", "at": [320, 288] }
      ],
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

* 图块字段：`texture` / `solid` / `overlay` / `trigger`，全部可省略。
* 实体字段：`id` / `behavior` / `tag` / `texture` / `visible` / `at:[x,y]` / `size:[w,h]` / `src:[x,y,w,h]` / `collider:[x,y,w,h]` / `params`。
* 场景字段：`view` / `controller` / `map` / `tileset` / `spawns` / `player` / `entities` / `transitions` / `params`。

### 3. `SceneManager` —— 数据驱动控制器

`SceneManager` 是引擎侧控制器：读配置、加载地图、按出生点放置玩家、处理图块触发器转场、跟随相机、管理实体生命周期。游戏通过 `game.scenes()` 访问：

```cpp
game.scenes().LoadConfig("config.json");
game.scenes().Start();                             // 从 config 的 initial 场景开始
// 或 game.scenes().Start("village", "from_house1");

game.scenes().RequestScene("house1", "entrance");                    // 主动切场景
game.scenes().RequestTransition("boss", { {"spawn_x", 100}, {"spawn_y", 200} });  // 按触发器切
```

实体生命周期：

```cpp
Entity* e = ctx.manager->Spawn("GiftBox", "gift", 320, 288);  // 生成并执行 OnSpawn
ctx.manager->Destroy(e);                                       // 延迟回收（帧末统一清理）
for (Entity* e : ctx.manager->Entities()) { /* ... */ }
Entity* player = ctx.manager->Player();                        // tag == "player"
Entity* npc    = ctx.manager->FindByTag("npc");
Entity* door   = ctx.manager->FindById("door1");
```

其它查询：

```cpp
const std::vector<SDL_Rect>& obstacles = ctx.manager->Colliders();  // 当前地图阻挡格
TileMap* map = ctx.manager->Map();
ctx.manager->DrawWorld();                                            // 绘制地图 + 实体（含 Y 轴排序）
```

每帧顺序：更新所有 `Behavior` → 跑场景 `SceneController` → 回收死亡实体 → 检查图块触发器 → 相机跟随。
`automatic: false` 的转场不会被自动处理，交给游戏 Controller 调用 `RequestTransition`。

### 4. `SceneView` / `SceneController` —— MVC 场景脚本

视图只负责渲染，逻辑放在控制器；两者用 `SceneRegistry` 自注册，注册名与配置里的 `view` / `controller` 对应：

```cpp
#include "Engine/SceneController.h"
#include "Engine/SceneManager.h"
#include "Engine/SceneRegistry.h"
#include "Engine/SceneView.h"

class VillageController : public SceneController {
public:
    void OnEnter(SceneContext& ctx) override { /* 场景进入：读 params、准备状态 */ }
    void OnExit() override { /* 释放场景级资源 */ }
    void HandleEvent(SceneContext& ctx, SDL_Event& event) override { /* 可选 */ }
    void Update(SceneContext& ctx) override { /* 每帧场景级逻辑 */ }
};

class VillageView : public SceneView {
public:
    void Render(SceneContext& ctx) override {
        ctx.manager->DrawWorld();   // 地图 + 实体，已按 Y 排序
        // 叠加自己的 HUD / 过场
    }
};

static SceneRegistry::ControllerProxy c("VillageController", [] { return new VillageController(); });
static SceneRegistry::ViewProxy v("VillageView", [] { return new VillageView(); });
```

`SceneContext` 提供 `game / manager / controller / map / data / params`。

### 5. `Entity` / `Behavior` —— 组件化实体

所有物体都是同一个 `Entity`（`Transform` + `Sprite` + `Collider` + `tag`），差异靠挂在它上面的 `Behavior`：

```cpp
#include "Engine/Behavior.h"
#include "Engine/BehaviorRegistry.h"
#include "Engine/Entity.h"

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
    void HandleEvent(SceneContext& ctx, SDL_Event& event) override { /* 可选 */ }

private:
    bool collected = false;
};

static BehaviorRegistry::Proxy proxy("Collectible", [] { return new CollectibleBehavior(); });
```

`Entity` 字段：`id` / `behaviorName` / `tag` / `transform{x,y,w,h}` / `sprite{texture,src,flip}` / `collider{offset,enabled}` / `visible` / `alive` / `sortYOffset`。
`Entity::Bounds()` 返回碰撞盒；`Entity::SortKey()` 返回 Y 轴排序键。

`Animator` 由 `Entity` 持有并自动绑定 `sprite`。行为里按状态选动画，`Update(dt)` 推进；同名动画正在播放时不会重置进度：

```cpp
// 行为里：
self->animator.SetSheet(frameW, frameH, cols);            // 单帧尺寸 + 每行列数
self->animator.LoadClips(config.value("animations", json::object()));
self->animator.Play(moving ? "walk_down" : "idle");
self->animator.SetFlipX(facingLeft);
self->animator.Update(dt);                                // 由 Time::DeltaTime() 驱动
```

```jsonc
// 数据驱动：frames 是整张精灵表的全局帧号
"params": {
  "animations": {
    "idle":      { "frames": [0], "fps": 1, "loop": true },
    "walk_down": { "frames": [6, 7, 8, 9, 10, 11], "fps": 8.33, "loop": true }
  }
}
```

### 6. `TileSet` / `TileMap` —— 通用瓦片地图

图块集描述「每个数字对应什么纹理、是否阻挡、是否 overlay、触发哪个事件」，全部由数据决定：

```cpp
#include "Engine/TileMap.h"

TileSet tileSet;
tileSet.tileSize = 32;
tileSet.tiles[0] = { "grass.png" };                        // texture / solid / overlay / trigger
tileSet.tiles[1] = { "tree.png", true, true };             // 阻挡 + overlay
tileSet.tiles[7] = { "door.png", false, false, "door" };   // 触发器

TileMap map;
map.Load("village.map", tileSet);
map.DrawGround(Game::renderer, Game::camera);              // 只画背景层

const std::vector<SDL_Rect>& solid = map.Colliders();      // 所有阻挡格
std::vector<SDL_Rect> doors = map.TriggerRects("door");    // 所有同名触发器
const std::vector<TileSprite>& overlays = map.Overlays();  // 参与 Y 排序的图块
```

`.map` 同时支持空格分隔（`1 2 3`）与紧凑数字格式（`123`）。

> overlay 图块与实体的排序绘制统一由 `SceneManager::DrawWorld()` 负责，不要手动分别画两层。

### 7. `Physics` —— AABB 碰撞（查询 / 解析分离）

职责分三类，互不混淆：

* **Collision Query**：只查询，不改动。
* **Collision Resolution**：只对世界 solid 矩形（`TileMap::Colliders()`）做移动解析。
* **Trigger**：`Collider::isTrigger`，只做重叠查询，不参与移动解析；瓦片触发器仍由 `TileSet` 的 `"trigger"` + `SceneManager` 转场处理。

```cpp
#include "Engine/Physics.h"

// Query
bool hit = Physics::CheckCollision(rectA, rectB);        // 原始矩形相交
bool touchGift = Physics::Overlap(*player, *gift);       // 实体 vs 实体（含 layer/mask 过滤）
bool inZone    = Physics::Overlap(*player, tileRect);    // 实体 vs 任意矩形（瓦片触发器/区域）

// Resolution：stepX/stepY 是「本帧位移」，不是速度
Physics::MoveTopDown(*entity, stepX, stepY, ctx.manager->Colliders());
Physics::MovePlatformer(*entity, stepX, stepY, onGround, ctx.manager->Colliders());
```

`Collider` 字段：`offset` / `enabled` / `isTrigger` / `layer` / `mask`。
`layer` = 自己属于哪层，`mask` = 自己关心哪些层；`Physics::ShouldCollide(a, b)` 判断两者是否互相作用。
预定义层：`Layers::Player / Enemy / World / Projectile / Trigger`（bit flag）。

```cpp
// 玩家：物理上被世界阻挡，同时能感知敌人与触发器
self->collider.enabled = true;
self->collider.layer = Layers::Player;
self->collider.mask  = Layers::World | Layers::Enemy | Layers::Trigger;

// 可拾取物 / 礼物盒：触发器，只对玩家生效
self->collider.enabled = true;
self->collider.isTrigger = true;
self->collider.layer = Layers::Trigger;
self->collider.mask  = Layers::Player;
```

移动解析作用于 `Entity` 与地图阻挡格；实体之间目前只做重叠查询（无刚体动力学）。

### 8. `ResourceManager` —— 资源注入

引擎从**内存资源表**加载资源，表由游戏在启动时注入，引擎不关心资源来源（embedded / file / pack）：

```cpp
#include "Engine/ResourceManager.h"

// 游戏侧：注册自己的资源表
ResourceManager::SetResourceTable(EMBEDDED_ASSETS);
ResourceManager::Init();

// 之后即可通过 ID 获取
SDL_Texture* tex = ResourceManager::GetTexture("player.png");  // 命中缓存不重复创建
std::string  map = ResourceManager::GetText("level1.map");
const EmbeddedResource* raw = ResourceManager::GetResource("player.png");

bool ok = ResourceManager::Has("player.png");
ResourceManager::Unload("player.png");  // 卸载单个纹理
ResourceManager::Clear();               // 释放所有缓存纹理（须在 DestroyRenderer 之前）
```

生命周期：资源表由游戏注入（通常 static inline），生命周期必须长于引擎；纹理按 ID 缓存，
`Unload` 卸载单个、`Clear` 清空全部；`Game::clean()` 已在销毁渲染器前调用 `Clear()`。

资源表类型为 `ResourceTable = std::map<std::string, EmbeddedResource>`，
其中 `EmbeddedResource { const unsigned char* data; size_t size; }`。

### 9. `Input` / `TextRenderer` / `Log`

```cpp
// 输入：action 映射（每帧由 Game::update() 刷新；底层仍可用 Input::IsKeyDown(SDL_Scancode)）
#include "Engine/Input.h"
if (Input::Down("MoveLeft"))  { /* 持续按住 */ }
if (Input::Pressed("Jump"))   { /* 本帧刚按下 */ }
if (Input::Released("Pause")) { /* 本帧刚松开 */ }
int h = Input::Axis("Horizontal");   // -1 / 0 / 1
// action 绑定来自 config.json 的 "input" 段（见下方最小示例）

// 文字渲染
#include "Engine/TextRenderer.h"
TextRenderer::Init("font.ttf", 24);                            // 字体资源 ID + 字号
TextRenderer::DrawText(10, 10, "Hello", {255, 255, 255, 255});
TextRenderer::Clean();

// 日志
#include "Engine/Log.h"
LOG_INFO("资源数量: " << count);
LOG_ERROR("加载失败: " << id);
```

### 10. Y 轴深度排序（遮挡）

按物体“脚底”的世界 Y 排序：Y 小的先画（在后面），Y 大的后画（在前面）。

* 地图在 tileset 里用 `"overlay": true` 标记参与排序的图块（树、房子、门、栅栏），其余为 `ground` 背景；`TileMap::Overlays()` 暴露它们，排序键 = 图块底部 Y。
* 实体排序键 = `Entity::SortKey()`（默认 `transform.y + transform.h`，可用 `sortYOffset` 微调）。
* `SceneManager::DrawWorld()` 先画 ground，再把 overlay 图块与所有可见实体合并成一个列表做稳定排序后绘制。

```jsonc
"3": { "texture": "tree.png", "solid": true, "overlay": true },
"4": { "texture": "wall.png", "solid": true, "overlay": true },
"5": { "texture": "roof.png", "solid": true, "overlay": true }
```

这样玩家走到树/房子的北面会被挡住，走到南面则盖住它们。

---

## 🎯 最小可运行示例

一个数据驱动的最小游戏：地图 + 一个 WASD 移动的玩家。

`src/PlayerBehavior.cpp`

```cpp
#include "Engine/Behavior.h"
#include "Engine/BehaviorRegistry.h"
#include "Engine/Entity.h"
#include "Engine/Input.h"
#include "Engine/Physics.h"
#include "Engine/SceneManager.h"

class PlayerBehavior : public Behavior {
public:
    void Update(SceneContext& ctx, float dt) override {
        int dx = 0, dy = 0;
        if (Input::Down("MoveLeft"))  dx -= 1;
        if (Input::Down("MoveRight")) dx += 1;
        if (Input::Down("MoveUp"))    dy -= 1;
        if (Input::Down("MoveDown"))  dy += 1;

        float stepX = dx * speed * dt;   // speed 单位为像素/秒
        float stepY = dy * speed * dt;
        if (stepX != 0.0f || stepY != 0.0f) {
            self->transform.x += stepX;
            self->transform.y += stepY;
            Physics::MoveTopDown(*self, stepX, stepY, ctx.manager->Colliders());
        }
    }

    float speed = 180.0f;
};

static BehaviorRegistry::Proxy proxy_player("Player", [] { return new PlayerBehavior(); });
```

`src/WorldView.cpp`

```cpp
#include "Engine/SceneManager.h"
#include "Engine/SceneRegistry.h"
#include "Engine/SceneView.h"

class WorldView : public SceneView {
public:
    void Render(SceneContext& ctx) override {
        ctx.manager->DrawWorld();
    }
};

static SceneRegistry::ViewProxy proxy_world("WorldView", [] { return new WorldView(); });
```

`src/main.cpp`

```cpp
#include "Engine/Config.h"
#include "Engine/Game.h"
#include "Engine/ResourceManager.h"
#include "EmbeddedAssets.h"

int main() {
    ResourceManager::SetResourceTable(EMBEDDED_ASSETS);
    ResourceManager::Init();

    Game game;
    game.init("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
              EngineConfig::SCREEN_WIDTH, EngineConfig::SCREEN_HEIGHT, false);

    if (!game.scenes().LoadConfig("config.json")) {
        game.clean();
        return 1;
    }
    game.scenes().Start();

    const int frameDelay = 1000 / EngineConfig::TARGET_FPS;
    while (game.running()) {
        Uint32 start = SDL_GetTicks();
        game.handleEvents();
        game.update();
        game.render();
        Uint32 elapsed = SDL_GetTicks() - start;
        if (elapsed < static_cast<Uint32>(frameDelay)) SDL_Delay(frameDelay - elapsed);
    }

    game.clean();
    return 0;
}
```

`config.json`（与 `EmbeddedAssets.h` 一起打包注入）

```jsonc
{
  "initial": { "scene": "world", "spawn": "default" },
  "input": {
    "actions": {
      "MoveLeft":  ["A", "LEFT"],
      "MoveRight": ["D", "RIGHT"],
      "MoveUp":    ["W", "UP"],
      "MoveDown":  ["S", "DOWN"],
      "Jump":      ["SPACE"],
      "Interact":  ["E", "RETURN"],
      "Pause":     ["ESCAPE"]
    },
    "axes": {
      "Horizontal": ["MoveLeft", "MoveRight"],
      "Vertical":   ["MoveUp", "MoveDown"]
    }
  },
  "tilesets": {
    "overworld": {
      "tile_size": 32,
      "tiles": {
        "0": { "texture": "grass.png" },
        "1": { "texture": "tree.png", "solid": true, "overlay": true }
      }
    }
  },
  "scenes": {
    "world": {
      "view": "WorldView",
      "map": "world.map",
      "tileset": "overworld",
      "spawns": { "default": [100, 100] },
      "player": {
        "behavior": "Player",
        "tag": "player",
        "texture": "player.png",
        "size": [32, 32]
      }
    }
  }
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

---

## 📄 License

引擎代码可自由使用。`engine/include/Engine/json.hpp` 为 [nlohmann/json](https://github.com/nlohmann/json)（MIT License）。
