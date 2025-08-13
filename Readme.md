My3DEngine/
├── CMakeLists.txt         # 主构建文件
├── external/              # 第三方依赖（bgfx、glm、etc.）
├── src/
│   ├── main.cpp           # 程序入口
│   ├── Engine/            # 核心引擎代码
│   │   ├── Engine.h
│   │   ├── Engine.cpp
│   │   ├── Renderer/      # 渲染相关
            |-BasicPipeline.cpp
            |- BasicPipeline.h
│   │   ├── Scene/         # 场景管理
│   │   ├── Input/         # 输入处理
│   │   ├── Utils/         # 工具类（日志、文件系统）
            |-MeshGen.cpp
            |-MeshGen.h
│   └── Game/              # 游戏或 Demo 逻辑
│       ├── DemoScene.cpp
│       ├── DemoScene.h
└── assets/                # 模型、纹理等资源
    ｜-fs_simple.sc
     |-varying.def.sc
     |-vs_simple.sc

