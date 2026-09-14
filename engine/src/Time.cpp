#include "Engine/Time.h"
#include <SDL.h>

namespace {
    // 单帧最大 dt：防止拖拽窗口 / 断点 / 卡顿导致异常大的位移
    constexpr float MAX_DELTA_TIME = 0.1f;

    Uint64 lastCounter = 0;
    Uint64 frequency = 0;

    float deltaTime = 0.0f;
    float unscaledDeltaTime = 0.0f;
    double elapsedTime = 0.0;
    uint64_t frameCount = 0;
    float timeScale = 1.0f;
}

namespace Time {

void Reset() {
    frequency = SDL_GetPerformanceFrequency();
    lastCounter = SDL_GetPerformanceCounter();
    deltaTime = 0.0f;
    unscaledDeltaTime = 0.0f;
}

void Tick() {
    if (frequency == 0) Reset();

    Uint64 now = SDL_GetPerformanceCounter();
    float dt = static_cast<float>(now - lastCounter) / static_cast<float>(frequency);
    lastCounter = now;

    if (dt > MAX_DELTA_TIME) dt = MAX_DELTA_TIME;
    if (dt < 0.0f) dt = 0.0f;

    unscaledDeltaTime = dt;
    deltaTime = dt * timeScale;
    elapsedTime += deltaTime;
    ++frameCount;
}

float DeltaTime() { return deltaTime; }
float UnscaledDeltaTime() { return unscaledDeltaTime; }
double ElapsedTime() { return elapsedTime; }
uint64_t FrameCount() { return frameCount; }
float TimeScale() { return timeScale; }

void SetTimeScale(float scale) {
    timeScale = scale < 0.0f ? 0.0f : scale;
}

}
