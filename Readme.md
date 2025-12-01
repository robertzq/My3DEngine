# 🎂 Birthday RPG (My3DEngine)

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

```text
.
├── assets/             # 游戏资源 (图片, 字体, 地图文件, JSON配置)
├── src/
│   ├── Core/           # 引擎核心 (Game loop, Scene基类, 渲染器)
│   ├── Entities/       # 游戏实体 (Player, GiftBox, GameObject)
│   ├── Map/            # 地图解析与渲染
│   ├── Physics/        # 简单的 AABB 碰撞检测库
│   └── Scenes/         # 具体游戏场景 (VillageScene, BattleScene 等)
├── CMakeLists.txt      # CMake 构建配置
└── runGame.sh          # 快速启动脚本
```
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
  python tools/embed_assets.py
```
生成资源
# 1. 创建构建目录
mkdir build
cd build

# 2. 生成 Makefile
cmake ..

# 3. 编译
make
3. 运行游戏 (Run)
方式 A：直接运行二进制文件 请确保在项目根目录下运行（以便程序能正确加载 assets/ 路径）：

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