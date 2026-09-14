#include "Engine/Animator.h"
#include "Engine/Entity.h"
#include "Engine/Log.h"

void Animator::Bind(Sprite* target) { sprite = target; }

void Animator::SetSheet(int frameWidth_, int frameHeight_, int columns_) {
    frameWidth = frameWidth_;
    frameHeight = frameHeight_;
    columns = columns_ > 0 ? columns_ : 1;
}

void Animator::AddClip(const std::string& name, const AnimationClip& clip) {
    clips[name] = clip;
}

void Animator::LoadClips(const json& animations) {
    if (!animations.is_object()) return;
    for (auto it = animations.begin(); it != animations.end(); ++it) {
        AnimationClip clip;
        const json& j = it.value();
        if (j.contains("frames") && j["frames"].is_array()) {
            for (const auto& frame : j["frames"]) {
                if (frame.is_number_integer()) clip.frames.push_back(frame.get<int>());
            }
        }
        clip.fps = j.value("fps", 8.0f);
        clip.loop = j.value("loop", true);
        clips[it.key()] = clip;
    }
}

void Animator::Play(const std::string& name) {
    if (name == current && playing) return;   // 同名动画不重置
    auto it = clips.find(name);
    if (it == clips.end()) return;
    current = name;
    frameIndex = 0;
    accumulator = 0.0f;
    playing = true;
    Apply();
}

void Animator::Stop() { playing = false; }

void Animator::SetSpeed(float scale) { speedScale = scale < 0.0f ? 0.0f : scale; }

void Animator::SetFlipX(bool flip) { flipX = flip; }
void Animator::SetFlipY(bool flip) { flipY = flip; }

const AnimationClip* Animator::CurrentClip() const {
    auto it = clips.find(current);
    return it == clips.end() ? nullptr : &it->second;
}

void Animator::Update(float deltaTime) {
    const AnimationClip* clip = CurrentClip();
    if (playing && clip && !clip->frames.empty() && clip->fps > 0.0f) {
        accumulator += deltaTime * speedScale;
        const float frameDuration = 1.0f / clip->fps;
        while (accumulator >= frameDuration) {
            accumulator -= frameDuration;
            if (frameIndex + 1 < static_cast<int>(clip->frames.size())) {
                ++frameIndex;
            } else if (clip->loop) {
                frameIndex = 0;
            } else {
                frameIndex = static_cast<int>(clip->frames.size()) - 1;
                playing = false;
                break;
            }
        }
    }
    Apply();
}

void Animator::Apply() {
    if (!sprite) return;

    const AnimationClip* clip = CurrentClip();
    if (clip && !clip->frames.empty() && frameWidth > 0 && frameHeight > 0) {
        int id = clip->frames[frameIndex < static_cast<int>(clip->frames.size()) ? frameIndex : 0];
        if (id < 0) id = 0;
        sprite->src.x = (id % columns) * frameWidth;
        sprite->src.y = (id / columns) * frameHeight;
        sprite->src.w = frameWidth;
        sprite->src.h = frameHeight;
    }

    sprite->flip = static_cast<SDL_RendererFlip>((flipX ? SDL_FLIP_HORIZONTAL : 0) |
                                                 (flipY ? SDL_FLIP_VERTICAL : 0));
}
