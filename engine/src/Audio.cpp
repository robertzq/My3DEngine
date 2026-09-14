#include "Engine/Audio.h"
#include "Engine/Log.h"
#include "Engine/ResourceManager.h"
#include <SDL_mixer.h>
#include <map>

namespace {

bool initialized = false;

std::map<std::string, Mix_Chunk*> sfxCache;
std::map<std::string, Mix_Music*> musicCache;

std::string currentMusic;
float musicVolume = 1.0f;
float sfxVolume = 1.0f;

int ToMixVolume(float volume) {
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    return static_cast<int>(volume * MIX_MAX_VOLUME + 0.5f);
}

Mix_Chunk* GetOrLoadSFX(const std::string& id) {
    auto it = sfxCache.find(id);
    if (it != sfxCache.end()) return it->second;
    if (!initialized) return nullptr;

    const EmbeddedResource* res = ResourceManager::GetResource(id);
    if (!res || res->data == nullptr || res->size == 0) {
        LOG_ERROR("Audio: 找不到音效资源 -> " << id);
        return nullptr;
    }

    SDL_RWops* rw = SDL_RWFromConstMem(res->data, static_cast<int>(res->size));
    if (!rw) {
        LOG_ERROR("Audio: SDL_RWFromConstMem 失败 -> " << id);
        return nullptr;
    }

    Mix_Chunk* chunk = Mix_LoadWAV_RW(rw, 1);   // freesrc=1
    if (!chunk) {
        LOG_ERROR("Audio: Mix_LoadWAV_RW 失败 (" << id << "): " << Mix_GetError());
        return nullptr;
    }
    sfxCache[id] = chunk;
    return chunk;
}

Mix_Music* GetOrLoadMusic(const std::string& id) {
    auto it = musicCache.find(id);
    if (it != musicCache.end()) return it->second;
    if (!initialized) return nullptr;

    const EmbeddedResource* res = ResourceManager::GetResource(id);
    if (!res || res->data == nullptr || res->size == 0) {
        LOG_ERROR("Audio: 找不到音乐资源 -> " << id);
        return nullptr;
    }

    SDL_RWops* rw = SDL_RWFromConstMem(res->data, static_cast<int>(res->size));
    if (!rw) {
        LOG_ERROR("Audio: SDL_RWFromConstMem 失败 -> " << id);
        return nullptr;
    }

    Mix_Music* music = Mix_LoadMUS_RW(rw, 1);   // freesrc=1
    if (!music) {
        LOG_ERROR("Audio: Mix_LoadMUS_RW 失败 (" << id << "): " << Mix_GetError());
        return nullptr;
    }
    musicCache[id] = music;
    return music;
}

}

bool Audio::Init() {
    if (initialized) return true;

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0) {
        LOG_ERROR("Audio: Mix_OpenAudio 失败: " << Mix_GetError());
        return false;
    }

    // OGG / MP3 music 需要显式初始化对应解码器；失败不致命（WAV 仍可用）
    int flags = MIX_INIT_OGG | MIX_INIT_MP3;
    int inited = Mix_Init(flags);
    if ((inited & flags) != flags) {
        LOG_WARN("Audio: 部分音乐解码器未初始化: " << Mix_GetError());
    }

    initialized = true;
    LOG_INFO("Audio 初始化完成");
    return true;
}

void Audio::Clean() {
    if (!sfxCache.empty() || !musicCache.empty()) {
        Mix_HaltMusic();
        for (auto& pair : sfxCache) Mix_FreeChunk(pair.second);
        for (auto& pair : musicCache) Mix_FreeMusic(pair.second);
        sfxCache.clear();
        musicCache.clear();
    }

    if (initialized) {
        Mix_CloseAudio();
        Mix_Quit();
        initialized = false;
    }
    currentMusic.clear();
}

bool Audio::Ready() { return initialized; }

bool Audio::LoadMusic(const std::string& id) { return GetOrLoadMusic(id) != nullptr; }
bool Audio::LoadSFX(const std::string& id) { return GetOrLoadSFX(id) != nullptr; }

void Audio::PlayMusic(const std::string& id, int loops) {
    // 同一首正在播放则不重启
    if (currentMusic == id && Mix_PlayingMusic()) return;

    Mix_Music* music = GetOrLoadMusic(id);
    if (!music) return;

    Mix_HaltMusic();
    if (Mix_PlayMusic(music, loops) == 0) {
        currentMusic = id;
        Mix_VolumeMusic(ToMixVolume(musicVolume));
    } else {
        LOG_ERROR("Audio: Mix_PlayMusic 失败 (" << id << "): " << Mix_GetError());
    }
}

void Audio::StopMusic() {
    Mix_HaltMusic();
    currentMusic.clear();
}

void Audio::PauseMusic() { Mix_PauseMusic(); }
void Audio::ResumeMusic() { Mix_ResumeMusic(); }

void Audio::PlaySFX(const std::string& id, int loops) {
    Mix_Chunk* chunk = GetOrLoadSFX(id);
    if (!chunk) return;
    Mix_VolumeChunk(chunk, ToMixVolume(sfxVolume));
    Mix_PlayChannel(-1, chunk, loops);
}

void Audio::SetMusicVolume(float volume) {
    musicVolume = volume < 0.0f ? 0.0f : (volume > 1.0f ? 1.0f : volume);
    Mix_VolumeMusic(ToMixVolume(musicVolume));
}

void Audio::SetSFXVolume(float volume) {
    sfxVolume = volume < 0.0f ? 0.0f : (volume > 1.0f ? 1.0f : volume);
    Mix_Volume(-1, ToMixVolume(sfxVolume));
}

float Audio::MusicVolume() { return musicVolume; }
float Audio::SFXVolume() { return sfxVolume; }
