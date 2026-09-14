# My2DEngine

一个轻量的 **C++17 + SDL2** 2D 游戏引擎，以及一个用来验证引擎机制的演示游戏（birthday RPG）。

引擎设计思想：**引擎只提供通用机制，不包含任何游戏逻辑、美术资源或关卡数据**。
地图加载、场景定义、场景切换全部由引擎负责，并且通过 JSON 配置文件驱动。
游戏侧只需要写「场景脚本」——即每帧的渲染代码，逻辑交给 Controller。

---

## ✨ 架构（MVC）

```text
Model       (引擎)   SceneData / TileSet / TileMap      ← 由 config.json 解析出的数据
Controller  (引擎)   SceneManager                       ← 地图加载、出生点、触发器、转场、相机
Controller  (游戏)   SceneController 子类                ← 可选：玩家移动、战斗状态机、过场演出
View        (游戏)   SceneView 子类                      ← 场景脚本，只负责每帧渲染
```

* 场景脚本（View）不碰地图解析、不写 `ChangeScene`、不管相机，只实现 `Render()`。
* 场景之间的跳转写在配置文件里：踩到某个图块触发器 → 引擎自动切到目标场景和出生点。
* 游戏需要动态转场时，Controller 调用 `context.manager->RequestScene(...)` / `RequestTransition(...)`。

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
│   ├── SceneContext.h    # 传给 View/Controller 的运行时上下文
│   ├── SceneRegistry.h   # View/Controller 自注册工厂
│   ├── GameObject.h / Physics.h / ResourceManager.h / TextRenderer.h / Input.h
│   └── Config.h / Log.h / json.hpp
└── src/                  # 引擎实现

game/                     # 演示游戏（只是引擎的使用者）
├── assets/config.json    # 数据驱动核心：tilesets + scenes + transitions
├── src/main.cpp          # 只做：注入资源、初始化、交给 SceneManager
├── src/Views/            # 场景脚本（VillageView / BattleView / PlayView）
├── src/Controllers/      # 游戏逻辑（VillageController / BattleController / PlayController）
├── src/RPGPlayer.* 等    # 游戏实体
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

* `view` / `controller` 填注册名，引擎用 `SceneRegistry` 创建。
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
3. 用 `SceneRegistry::ViewProxy` / `ControllerProxy` 自注册。
4. 在 `config.json` 里定义 tileset、场景、出生点与转场。

引擎代码无需改动。
