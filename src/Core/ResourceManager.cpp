#include "ResourceManager.h"
#include "EmbeddedAssets.h" // 刚刚生成的头文件
#include "Game.h" // 需要用到 Game::renderer
#include <SDL_image.h>
#include <sstream>

std::map<std::string, SDL_Texture*> ResourceManager::textureCache;

void ResourceManager::Init() {
    // 这里可以打印一下有多少内嵌资源
    std::cout << "[ResourceManager] 初始化完成。内嵌资源数量: " << EMBEDDED_ASSETS.size() << std::endl;
}

SDL_Texture* ResourceManager::GetTexture(const std::string& id) {
    // 1. 先查缓存
    auto it = textureCache.find(id);
    if (it != textureCache.end()) {
        return it->second;
    }

    // 2. 缓存没有，尝试加载
    SDL_Texture* tex = LoadTextureFromMemory(id);
    if (tex) {
        textureCache[id] = tex;
    }
    return tex;
}

SDL_Texture* ResourceManager::LoadTextureFromMemory(const std::string& id) {
    // 1. 在内嵌资源表中查找
    auto it = EMBEDDED_ASSETS.find(id);
    if (it == EMBEDDED_ASSETS.end()) {
        std::cerr << "[ResourceManager] 错误: 找不到内嵌资源 -> " << id << std::endl;
        return nullptr;
    }

    // 2. 获取数据指针和大小
    const unsigned char* data = it->second.data;
    size_t size = it->second.size;

    // 3. 创建 SDL_RWops (内存流)
    SDL_RWops* rw = SDL_RWFromConstMem(data, (int)size);
    if (!rw) {
        std::cerr << "SDL_RWFromConstMem 失败: " << SDL_GetError() << std::endl;
        return nullptr;
    }

    // 4. 使用 IMG_Load_RW 从内存流加载图片
    // 最后一个参数 1 表示加载完自动关闭 rw
    SDL_Surface* surface = IMG_Load_RW(rw, 1);
    if (!surface) {
        std::cerr << "IMG_Load_RW 失败 (" << id << "): " << IMG_GetError() << std::endl;
        return nullptr;
    }

    // 5. 转为 Texture
    SDL_Texture* tex = SDL_CreateTextureFromSurface(Game::renderer, surface);
    SDL_FreeSurface(surface);

    return tex;
}

std::string ResourceManager::GetTextContent(const std::string& id) {
    auto it = EMBEDDED_ASSETS.find(id);
    if (it == EMBEDDED_ASSETS.end()) {
        std::cerr << "[ResourceManager] 错误: 找不到文本资源 -> " << id << std::endl;
        return "";
    }

    // 直接用数据构造 string
    // 注意：二进制数据可能不包含 \0 结尾，所以必须指定长度
    return std::string(reinterpret_cast<const char*>(it->second.data), it->second.size);
}

void ResourceManager::Clean() {
    for (auto& pair : textureCache) {
        SDL_DestroyTexture(pair.second);
    }
    textureCache.clear();
    std::cout << "[ResourceManager] 已清理所有纹理缓存" << std::endl;
}

const EmbeddedResource* ResourceManager::GetResource(const std::string& id) {
    auto it = EMBEDDED_ASSETS.find(id);
    if (it == EMBEDDED_ASSETS.end()) {
        std::cerr << "[ResourceManager] 错误: 找不到内嵌资源 -> " << id << std::endl;
        return nullptr;
    }
    return &(it->second);
}