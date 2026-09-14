#pragma once
#include <cstdint>

// 统一帧时间模型：引擎每帧调用一次 Time::Tick()，其余系统只读。
//
// * DeltaTime()          —— 本帧时间（受 TimeScale 影响），供 gameplay / 动画使用
// * UnscaledDeltaTime()  —— 真实帧时间（不受 TimeScale 影响），供 UI / 暂停菜单使用
// * ElapsedTime()        —— 累计游戏时间（受 TimeScale 影响），单位秒
// * FrameCount()         —— 已推进的帧数
// * TimeScale()          —— 全局时间倍率，默认 1.0；设为 0 可暂停
namespace Time {
    // 重置时钟基准（引擎初始化、场景切换时调用，避免加载耗时被计入下一帧）
    void Reset();

    // 推进一帧：计算 dt、累加时间、递增帧计数。由 Game::update() 每帧调用一次。
    void Tick();

    float DeltaTime();
    float UnscaledDeltaTime();
    double ElapsedTime();
    uint64_t FrameCount();
    float TimeScale();
    void SetTimeScale(float scale);
}
