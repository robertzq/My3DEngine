# My2DEngine

基于 C++ 和 SDL2 开发的 2D RPG 游戏引擎演示项目。
本项目包含了一个完整的 RPG 游戏流程，展示了场景切换、地图加载、碰撞检测以及回合制战斗系统。

## ✨ 功能特性 (Features)

* **核心引擎架构**：
    * 基于 `Scene` 的状态管理（支持场景堆栈和切换）。
    * 单例模式的 `Game` 核心控制器。
    * 资源管理系统 (`ResourceManager`)，支持纹理缓存。
* **游戏性**：
    * **RPG 探索**：支持顶视角的 2D 地图探索。
    * **场景切换**：支持从村庄进入房屋、从房屋触发战斗等场景流转。
    * **地图系统**：支持加载 `.map` 文本格式的地图文件（包含墙壁、装饰、触发器层）。
    * **战斗系统**：触发式 BOSS 战（Slime, Goblin, King），支持战斗结束后返回原地图位置。
* **技术栈**：
    * **语言**：C++17
    * **构建工具**：CMake (跨平台支持 Windows & macOS)
    * **图形库**：SDL2, SDL2_image, SDL2_ttf
    * **数据解析**：nlohmann/json

## 📂 项目结构

引擎与游戏完全分离：`engine/` 是可复用的引擎库，`game/` 是具体的游戏项目。

```text
.
├── engine/                 # 可复用的 2D 游戏引擎（独立编译成静态库 libengine.a）
│   ├── include/Engine/     # 引擎公共 API 头文件
│   │   ├── Game.h          #   游戏循环 + 场景管理
│   │   ├── Scene.h         #   场景基类
│   │   ├── SceneFactory.h  #   场景自注册工厂
│   │   ├── GameObject.h    #   通用游戏对象基类
│   │   ├── ResourceManager.h # 资源加载 + 缓存 + 绘制
│   │   ├── TextRenderer.h  #   文字渲染（SDL_ttf）
│   │   ├── Input.h         #   输入轮询
│   │   ├── Physics.h       #   AABB 碰撞检测
│   │   └── (Config / Log / json.hpp) ...
│   └── src/                # 引擎实现
├── game/                   # 生日 RPG 游戏（链接引擎库）
│   ├── src/                # 游戏逻辑（main、实体、地图、GameState）
│   │   └── Scenes/         #   具体游戏场景 (VillageScene, BattleScene 等)
│   ├── assets/             # 游戏资源 (图片, 字体, 地图文件, JSON配置)
│   └── tools/              # 资源打包脚本 (embed_assets.py)
├── CMakeLists.txt          # 顶层 CMake 构建配置
└── runGame.sh              # 快速启动脚本
```

> 写新游戏只需新建一个 `game/` 目录，`#include "Engine/..."` 并链接 `engine` 库，
> 通过 `ResourceManager::SetResourceTable()` 注入自己的资源即可，引擎代码无需改动。
## 🚀 快速开始 (Getting Started)
1. 环境依赖 (Prerequisites)
本项目依赖 SDL2 系列库。

macOS (Homebrew):

Bash
```shell
    brew install cmake
    brew install sdl2
    brew install sdl2_image
    brew install sdl2_ttf
```

Windows: 推荐使用 vcpkg 安装 SDL2 库，或者下载开发包并配置环境变量。 注：本项目 CMakeLists.txt 已包含针对 Windows MSVC 的 UTF-8 编码修复及路径自动检测。

2. 构建项目 (Build)
在项目根目录下执行以下命令：

Bash
```shell
# 1. 创建构建目录
mkdir build
cd build

# 2. 生成 Makefile
cmake ..

# 3. 编译（会先构建 engine 静态库，再链接 game；同时自动嵌入游戏资源）
make
```

> 游戏资源由 `game/tools/embed_assets.py` 在构建时自动打包成 `EmbeddedAssets.h`（带增量判断），
> 无需手动执行该脚本。

3. 运行游戏 (Run)
方式 A：直接运行二进制文件（资源已内嵌，无需依赖 assets 路径）：

Bash
```shell
  ./build/MyEngine
```

方式 B：使用脚本 (macOS/Linux)
```shell
  ./runGame.sh
```

🎮 操作说明 (Controls)
移动：W A S D

交互/确认：(根据具体逻辑补充，例如空格或回车)

退出：点击窗口关闭按钮

### 🛠️ 开发日志
渲染循环：Input -> Update -> Render -> Delay (60 FPS Lock).

### 场景流：

VillageScene: 初始场景，包含 NPC 和房屋入口。

HouseScene: 室内场景，包含 BOSS 触发区域。

BattleScene: 战斗场景，回合制逻辑。

Created by Robertzq