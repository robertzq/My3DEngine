#pragma once
#include <map>
#include <string>
#include <vector>
#include <SDL.h>
#include "Engine/Texture.h"

// 内嵌资源描述：数据指针 + 大小（由游戏侧通过 SetResourceTable 注册）
struct EmbeddedResource {
    const unsigned char* data;
    size_t size;
};

// 资源表类型：资源 ID -> 资源数据
using ResourceTable = std::map<std::string, EmbeddedResource>;

// 资源管理：从内存资源表按 ID 加载纹理 / 文本，并缓存纹理。
//
// 生命周期约定：
// * 资源表由游戏注入（通常为 static inline），生命周期必须长于引擎；
// * 纹理（Texture）由本类拥有，命中缓存不会重复创建；Unload 卸载单个，Clear 清空全部；
// * Clear 必须在 Renderer::Clean()（GL context 销毁）之前调用（Game::clean() 已保证）。
// 引擎不关心资源来自 embedded / file / pack，当前后端为 EmbeddedResource。
class ResourceManager {
public:
    static void Init();
    static void SetResourceTable(const ResourceTable& table);

    // File 资源来源：资源表里找不到的 id，会回退到 basePath 下的同名文件读取。
    // 服务所有资源类型（Texture / Text / Audio / Shader），不是 Audio 特例。
    static void SetBasePath(const std::string& basePath);

    // 资源表中或 File 来源是否存在该 ID
    static bool Has(const std::string& id);

    // 纹理：命中缓存直接返回；否则从资源表加载并缓存（返回 non-owning 语义，所有权在 ResourceManager）
    static Texture* GetTexture(const std::string& id);

    // 文本 / 二进制内容（.map / .json / 字体数据）
    static std::string GetText(const std::string& id);
    static const EmbeddedResource* GetResource(const std::string& id);

    // 卸载 / 释放
    static void Unload(const std::string& id);
    static void Clear();

    // 绘制辅助（转调 Renderer；与资源生命周期无关）
    static void Draw(Texture* tex, SDL_Rect src, SDL_Rect dest, SDL_RendererFlip flip);
    static void DrawWhole(Texture* tex, SDL_Rect dest, SDL_RendererFlip flip);

private:
    static Texture* LoadTextureFromMemory(const std::string& id);

    static std::map<std::string, Texture*> textureCache;
    static const ResourceTable* resourceTable;

    // File 来源：读入的文件字节按 id 缓存，生命周期由 ResourceManager 保证
    static std::string basePath;
    static std::map<std::string, std::vector<unsigned char>> fileBuffers;
    static std::map<std::string, EmbeddedResource> fileResources;
};
