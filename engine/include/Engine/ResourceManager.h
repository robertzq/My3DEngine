#pragma once
#include <map>
#include <string>
#include <SDL.h>
#include <iostream>

// 内嵌资源描述：数据指针 + 大小（由游戏侧通过 SetResourceTable 注册）
struct EmbeddedResource {
    const unsigned char* data;
    size_t size;
};

// 资源表类型：资源 ID -> 资源数据
using ResourceTable = std::map<std::string, EmbeddedResource>;

// 这是一个单例或者全静态类
class ResourceManager {
public:
    // 初始化：如果需要做预加载可以在这里做
    static void Init();

    // 注册资源表：由游戏在启动时调用，把内嵌资源注入引擎
    static void SetResourceTable(const ResourceTable& table);

    // 获取纹理：如果已经加载过，直接返回；否则从资源表加载
    // id 是 assets 下的相对路径，如 "dirt.png"
    static SDL_Texture* GetTexture(const std::string& id);

    // 获取纯文本/数据内容 (比如读取 .map 文件)
    static std::string GetTextContent(const std::string& id);
    static const EmbeddedResource* GetResource(const std::string& id);

    // 清理所有缓存的纹理
    static void Clean();

    // 绘制辅助（原 TextureManager 的职责，合并到此处统一管理）
    static void Draw(SDL_Texture* tex, SDL_Rect src, SDL_Rect dest, SDL_Renderer* ren, SDL_RendererFlip flip);
    static void DrawWhole(SDL_Texture* tex, SDL_Rect dest, SDL_Renderer* ren, SDL_RendererFlip flip);

private:
    // 缓存加载好的纹理，避免重复创建
    static std::map<std::string, SDL_Texture*> textureCache;

    // 指向游戏提供的资源表（生命周期由游戏保证，通常是静态 inline map）
    static const ResourceTable* resourceTable;

    // 内部辅助：从内存加载纹理
    static SDL_Texture* LoadTextureFromMemory(const std::string& id);
};
