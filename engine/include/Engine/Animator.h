#pragma once
#include <SDL.h>
#include <map>
#include <string>
#include <vector>
#include "Engine/json.hpp"

using json = nlohmann::json;

struct Sprite;

// 一段帧动画：frames 是整张精灵表的全局帧号，按 columns 换算成 src 矩形。
struct AnimationClip {
    std::vector<int> frames;
    float fps = 8.0f;
    bool loop = true;
};

// 实体帧动画：由 Entity 持有，Update(dt) 直接把当前帧写入绑定 Sprite 的 src。
class Animator {
public:
    void Bind(Sprite* sprite);
    // 精灵表布局：单帧尺寸 + 每行列数（用于把全局帧号换算成行列）
    void SetSheet(int frameWidth, int frameHeight, int columns);

    void AddClip(const std::string& name, const AnimationClip& clip);
    // 从配置解析：{ "walk": { "frames": [0,1,2,3], "fps": 8, "loop": true }, ... }
    void LoadClips(const json& animations);

    // 播放：同名动画正在播放时不重置进度
    void Play(const std::string& name);
    void Stop();

    void SetSpeed(float scale);   // 动画速度倍率，默认 1.0
    void SetFlipX(bool flip);
    void SetFlipY(bool flip);
    bool FlipX() const { return flipX; }
    bool FlipY() const { return flipY; }

    void Update(float deltaTime);

    bool Playing() const { return playing; }
    const std::string& Current() const { return current; }
    const AnimationClip* CurrentClip() const;

private:
    void Apply();

    Sprite* sprite = nullptr;
    int frameWidth = 0;
    int frameHeight = 0;
    int columns = 1;

    std::map<std::string, AnimationClip> clips;
    std::string current;
    bool playing = false;
    int frameIndex = 0;
    float accumulator = 0.0f;
    float speedScale = 1.0f;

    bool flipX = false;
    bool flipY = false;
};
