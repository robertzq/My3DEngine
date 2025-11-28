#pragma once
#include <SDL.h>

class Scene {
public:
    virtual ~Scene() {}

    // 进入场景时调用（加载资源、创建对象）
    virtual void OnEnter() = 0;

    // 离开场景时调用（清理内存）
    virtual void OnExit() = 0;

    // 处理输入 (键盘鼠标)
    virtual void HandleEvents(SDL_Event& event) = 0;

    // 每一帧的逻辑更新
    virtual void Update() = 0;

    // 每一帧的渲染
    virtual void Render() = 0;
};