#pragma once
#include <map>
#include <string>
#include <SDL.h>
#include <iostream>

// 这是一个单例或者全静态类
class ResourceManager {
public:
    // 初始化：如果需要做预加载可以在这里做
    static void Init();

    // 获取纹理：如果已经加载过，直接返回；否则从 EmbeddedAssets 加载
    // id 是 assets 下的相对路径，如 "dirt.png"
    static SDL_Texture* GetTexture(const std::string& id);

    // 获取纯文本/数据内容 (比如读取 .map 文件)
    // 返回 string 方便处理，或者你可以返回 stringstream
    static std::string GetTextContent(const std::string& id);

    // 清理所有缓存的纹理
    static void Clean();

private:
    // 缓存加载好的纹理，避免重复创建
    static std::map<std::string, SDL_Texture*> textureCache;

    // 内部辅助：从内存加载纹理
    static SDL_Texture* LoadTextureFromMemory(const std::string& id);
};