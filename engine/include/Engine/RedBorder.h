#pragma once

// 屏幕边框红色危险 / 焦虑抖动（final post-process）。
struct RedBorderParams {
    float borderWidth = 0.12f;   // 边框权重范围（归一化）
    float shakeAmount = 0.012f;  // 高频抖动幅度
    float noiseSpeed = 2.0f;     // noise 时间速度
    float redStrength = 0.85f;   // 红色强度
    float chromatic = 0.004f;    // 边缘色差
};

class RedBorder {
public:
    // 注册 shader，并把一个 disabled 的 final effect 挂到 PostProcess
    static bool Init();
    static void Clean();

    // 触发：duration 秒内 fade in / hold / fade out，结束后自动关闭
    static void Trigger(float duration, const RedBorderParams& params = {});

    static void Update(float deltaTime);
    static bool Active();
};
