# 安装构建工具
brew install cmake

# 安装 SDL2 开发库
brew install sdl2
brew install sdl2_image  # 用于加载图片
brew install sdl2_ttf    # 用于显示文字

while (isRunning) {
    1. 处理输入 (Input)      // 玩家按了什么键？
    2. 更新状态 (Update)     // 根据按键和物理规则，物体移动多少？(这里就有 Rigidbody)
    3. 渲染画面 (Render)     // 把算好的新位置画到屏幕上
    4. 帧率控制 (Delay)      // 别跑太快，保持 60 FPS
}

mkdir build

cd build
cmake ..
make

in project root directory
./build/MyEngine