#pragma once
#include <string>

// 最小 2D 音频系统（SDL_mixer）。gameplay 只使用资源 ID。
//
// Ownership：
// * 原始资源 bytes 由游戏的 EmbeddedResource 表拥有，经 ResourceManager 访问（引擎非拥有）。
// * Mix_Chunk*（SFX）/ Mix_Music*（Music）由 Audio 系统拥有并缓存，Audio::Clean() 负责释放。
// * Entity / Behavior 不得持有 Mix_Chunk* / Mix_Music*。
//
// 生命周期：Game::init → Audio::Init()；Game::clean → Audio::Clean()
// （在 ResourceManager::Clear() 与 SDL_Quit() 之前）。Clean() 可重复安全调用。
//
// 行为约定：PlayMusic 传入与当前正在播放相同的 id 时不重启。
class Audio {
public:
    static bool Init();
    static void Clean();
    static bool Ready();

    // 惰性加载并缓存（重复调用同一 id 不会重复创建）
    static bool LoadMusic(const std::string& id);
    static bool LoadSFX(const std::string& id);

    static void PlayMusic(const std::string& id, int loops = -1);   // loops<0 无限循环
    static void StopMusic();
    static void PauseMusic();
    static void ResumeMusic();

    static void PlaySFX(const std::string& id, int loops = 0);

    // 音量 0.0 ~ 1.0（内部转换到 SDL_mixer 的 0~128）
    static void SetMusicVolume(float volume);
    static void SetSFXVolume(float volume);
    static float MusicVolume();
    static float SFXVolume();
};
