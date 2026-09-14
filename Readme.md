# My2DEngine

一个轻量的 **C++17 + SDL2** 2D 游戏引擎，以及一个用来验证引擎机制的演示游戏（birthday RPG）。

引擎设计思想：**引擎只提供通用机制，不包含任何游戏逻辑、美术资源或关卡数据**。
地图加载、场景定义、场景切换全部由引擎负责，并且通过 JSON 配置文件驱动。
游戏侧只需要写「场景脚本」——即每帧的渲染代码，逻辑交给 Controller。

---

## ✨ 架构（MVC + 组件化实体）

```text
Model       (引擎)   SceneData / TileSet / TileMap / Entity(Transform/Sprite/Collider)  ← config.json 解析
Controller  (引擎)   SceneManager                       ← 地图加载、出生点、触发器、转场、相机、实体生命周期
Controller  (游戏)   Behavior 子类                       ← 玩家移动、礼物盒、收集物等“实体逻辑”
Controller  (游戏)   SceneController 子类                ← 可选：场景级逻辑（过场演出、战斗状态机）
View        (游戏)   SceneView 子类                      ← 场景脚本，只负责每帧渲染
```

* **实体 = 数据**：`Entity` 只有 `transform / sprite / collider / tag` 和两个开关（visible/alive），
  任何物体都是它，不再为每个物体造一个类。
* **行为 = 逻辑**：`Behavior` 子类（`PlayerBehavior` / `GiftBoxBehavior` / `CollectibleBehavior`）
  按注册名挂到实体上，实现每帧逻辑；引擎用 `BehaviorRegistry` 创建。
* 场景脚本（View）不碰地图解析、不写 `ChangeScene`、不管相机，只实现 `Render()`。
* 场景跳转写在配置里：踩到图块触发器 → 引擎自动切到目标场景与出生点。
* 动态转场：Controller/Behavior 调 `context.manager->RequestScene(...)` / `RequestTransition(...)`。

---

## 📂 目录结构

```text
engine/
├── include/Engine/
│   ├── Game.h            # SDL 初始化 + 主循环，持有 SceneManager
│   ├── SceneManager.h    # 控制器：配置加载、场景切换、相机、触发器
│   ├── SceneData.h       # 数据模型：场景定义 / 转场 / 出生点
│   ├── TileSet.h         # 图块集：纹理、是否阻挡、触发器名
│   ├── TileMap.h         # 通用瓦片地图：解析 / 绘制 / 碰撞盒 / 触发器
│   ├── SceneView.h       # 视图基类（场景脚本，只渲染）
│   ├── SceneController.h # 控制器基类（游戏逻辑）
│   ├── SceneContext.h    # 传给 View/Controller/Behavior 的运行时上下文
│   ├── SceneRegistry.h   # View/Controller 自注册工厂
│   ├── Entity.h          # 通用实体：Transform / Sprite / Collider
│   ├── Behavior.h        # 行为基类（实体逻辑）
│   ├── BehaviorRegistry.h# 行为自注册工厂
│   ├── GameObject.h / Physics.h / ResourceManager.h / TextRenderer.h / Input.h
│   └── Config.h / Log.h / json.hpp
└── src/                  # 引擎实现

game/                     # 演示游戏（只是引擎的使用者）
├── assets/config.json    # 数据驱动核心：tilesets + scenes + transitions + player/entities
├── src/main.cpp          # 只做：注入资源、初始化、交给 SceneManager
├── src/Views/            # 场景脚本（VillageView / BattleView / PlayView / GiftBanner）
├── src/Controllers/      # 场景级逻辑（VillageController / BattleController / PlayController）
├── src/Behaviors/        # 实体行为（PlayerBehavior / GiftBoxBehavior / CollectibleBehavior）
└── tools/embed_assets.py # 资源打包
```

---

## ⚙️ 配置文件 `game/assets/config.json`

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
      "view": "VillageView",
      "controller": "VillageController",
      "map": "village.map",
      "tileset": "overworld",
      "spawns": { "default": [100, 100], "from_house1": [192, 230] },
      "player": {
        "behavior": "Player",
        "tag": "player",
        "texture": "rpgPlayer.png",
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

* 场景的 `player` 会被引擎按 `spawns` / 运行时坐标生成，并打上 `tag: "player"`，供相机和触发器使用。
* `entities` 数组可放置任意静态实体（同样按 `behavior` 注册名生成）。
* `view` / `controller` 填注册名，引擎用 `SceneRegistry` 创建；`behavior` 用 `BehaviorRegistry` 创建。
* `transitions[].automatic = false` 的触发器不会被引擎自动处理，交给游戏 Controller
  （例如 BOSS 战需要携带“返回坐标”这类运行时参数，用 `RequestTransition("boss", {...})`）。
* `spawn_x` / `spawn_y` 作为运行时参数可覆盖配置里的出生点，用于“战斗结束回到原地”。

---

## 🔁 数据驱动的转场流程

```text
玩家走到 trigger 图块
  → SceneManager::CheckTransitions() 命中 config 中的 transition
  → 按 target / spawn 加载新场景，重建 TileMap、按出生点放置玩家
  → 相机自动跟随并 clamp 到地图边界
```

游戏侧不再出现 `Map::LoadMap`、`ChangeScene(new XxxScene(...))`、硬编码坐标判断。

---

## 🧱 组件化实体与行为（Entity + Behavior）

所有物体都是同一个 `Entity`，差异靠挂在它上面的 `Behavior`：

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
    bool collected = false;
};

static BehaviorRegistry::Proxy proxy("Collectible", [] { return new CollectibleBehavior(); });
```

运行时增删与管理：

```cpp
context.manager->Spawn("GiftBox", "gift", 320, 288);       // 动态生成
context.manager->Destroy(entity);                          // 延迟回收
for (Entity* e : context.manager->Entities()) { /* ... */ }
Entity* player = context.manager->Player();                // tag == "player"
```

`SceneManager` 每帧：更新所有行为 → 跑场景 Controller → 回收死亡实体 → 检查触发器 → 相机跟随。
`SceneView` 只需 `context.manager->DrawWorld()` 再叠加自己的 HUD/过场。

---

## 🚀 构建与运行

```shell
brew install cmake sdl2 sdl2_image sdl2_ttf
./runGame.sh
# 或
mkdir -p build && cd build && cmake .. && make && cd .. && ./build/MyEngine
```

操作：`W A S D` 移动，`空格` 跳跃/确认，`回车` 交互。

---

## 🧩 写一个新游戏

1. 新建 `game/`，在 `main.cpp` 里 `ResourceManager::SetResourceTable(...)` 注入资源，
   再 `game.scenes().LoadConfig("config.json")` 与 `Start()`。
2. 写 `SceneView` 子类（只实现 `Render`）和可选的 `SceneController` 子类。
3. 写 `Behavior` 子类承载实体逻辑，用 `BehaviorRegistry::Proxy` 自注册。
4. 用 `SceneRegistry::ViewProxy` / `ControllerProxy` 注册视图与场景控制器。
5. 在 `config.json` 里定义 tileset、场景、出生点、转场与 `player` / `entities`。

引擎代码无需改动。
