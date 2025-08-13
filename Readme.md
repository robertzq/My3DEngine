```
My3DEngine
.
├── ./.gitignore
├── ./CMakeLists.txt
├── ./Readme.md
├── ./TREE.md
├── ./assets
│   └── ./assets/shaders
│       ├── ./assets/shaders/fs_simple.sc
│       ├── ./assets/shaders/varying.def.sc
│       └── ./assets/shaders/vs_simple.sc
├── ./bin
│   └── ./bin/assets
│       └── ./bin/assets/shaders_bin
│           ├── ./bin/assets/shaders_bin/fs_simple.pp.txt
│           └── ./bin/assets/shaders_bin/vs_simple.pp.txt
├── ./build
├── ./compileCode.sh
├── ./compileScript.sh
├── ./compileShader.sh
├── ./err.log
├── ./external
│   ├── ./external/bgfx.cmake
│   ├── ./external/glfw
│   └── ./external/glm
├── ./gen_tree.sh
├── ./runGame.sh
└── ./src
    ├── ./src/Engine
    │   ├── ./src/Engine/Engine.h
    │   ├── ./src/Engine/Engine.mm
    │   ├── ./src/Engine/Input
    │   ├── ./src/Engine/Renderer
    │   │   ├── ./src/Engine/Renderer/BasicPipeline.cpp
    │   │   └── ./src/Engine/Renderer/BasicPipeline.h
    │   ├── ./src/Engine/Scene
    │   ├── ./src/Engine/ShaderUtils.cpp
    │   ├── ./src/Engine/ShaderUtils.h
    │   └── ./src/Engine/Utils
    │       ├── ./src/Engine/Utils/MeshGen.cpp
    │       └── ./src/Engine/Utils/MeshGen.h
    ├── ./src/Game
    │   ├── ./src/Game/DemoScene.cpp
    │   └── ./src/Game/DemoScene.h
    └── ./src/main.cpp
```
