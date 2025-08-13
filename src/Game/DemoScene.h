// src/Game/DemoScene.h
#pragma once
#include <bgfx/bgfx.h>

class Engine;

class DemoScene {
public:
    void init(Engine* eng);
    void update();
    void render();
    void shutdown();

private:
    // === 这些是你在 .cpp 里用到的函数，必须在头文件里声明 ===
    void setMvpColor(const float model[16], const float color[4]);
    void buildRoom();
    void buildSphere(int stacks, int slices);

    // === 运行时句柄 ===
    bgfx::ProgramHandle      m_prog   = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle      u_mvp    = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle      u_color  = BGFX_INVALID_HANDLE;
    bgfx::VertexLayout       m_layout{};
    bgfx::VertexBufferHandle m_roomVbh = BGFX_INVALID_HANDLE;
    bgfx::IndexBufferHandle  m_roomIbh = BGFX_INVALID_HANDLE;
    bgfx::VertexBufferHandle m_ballVbh = BGFX_INVALID_HANDLE;
    bgfx::IndexBufferHandle  m_ballIbh = BGFX_INVALID_HANDLE;

    // === 场景参数（给出安全的默认值！） ===
    float m_roomHalf = 3.0f;   // 房间 XZ 半边长（单位：米）
    float m_ballR    = 0.6f;  // 小球半径
    float m_posX     = 0.0f;   // 球心 X（XZ 平面）
    float m_posZ     = 0.0f;   // 球心 Z
    float m_velX     = 0.6f;   // 初速度 X（m/s）
    float m_velZ     = 0.4f;   // 初速度 Z（m/s）
    float m_angle    = 0.0f;   // 球体自旋角（弧度）
    double m_lastTime = 0.0;   // 上一帧时间戳

    bool m_ready = false;
    Engine* m_eng = nullptr;
};
