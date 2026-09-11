#include "Engine/ResourceManager.h"
#include "Engine/Game.h" // 需要用到 Game::renderer
#include "Engine/Log.h"
#include <SDL_image.h>
#include <sstream>

std::map<std::string, SDL_Texture*> ResourceManager::textureCache;
const ResourceTable* ResourceManager::resourceTable = nullptr;

void ResourceManager::Init() {
    size_t count = resourceTable ? resourceTable->size() : 0;
    LOG_INFO("ResourceManager 初始化完成。内嵌资源数量: " << count);
}

void ResourceManager::SetResourceTable(const ResourceTable& table) {
    resourceTable = &table;
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
    if (!resourceTable) {
        LOG_ERROR("资源表未注册，无法加载 -> " << id);
        return nullptr;
    }

    // 1. 在资源表中查找
    auto it = resourceTable->find(id);
    if (it == resourceTable->end()) {
        LOG_ERROR("找不到内嵌资源 -> " << id);
        return nullptr;
    }

    // 2. 获取数据指针和大小
    const unsigned char* data = it->second.data;
    size_t size = it->second.size;

    // 3. 创建 SDL_RWops (内存流)
    SDL_RWops* rw = SDL_RWFromConstMem(data, (int)size);
    if (!rw) {
        LOG_ERROR("SDL_RWFromConstMem 失败: " << SDL_GetError());
        return nullptr;
    }

    // 4. 使用 IMG_Load_RW 从内存流加载图片
    // 最后一个参数 1 表示加载完自动关闭 rw
    SDL_Surface* surface = IMG_Load_RW(rw, 1);
    if (!surface) {
        LOG_ERROR("IMG_Load_RW 失败 (" << id << "): " << IMG_GetError());
        return nullptr;
    }

    // 5. 转为 Texture
    SDL_Texture* tex = SDL_CreateTextureFromSurface(Game::renderer, surface);
    SDL_FreeSurface(surface);

    return tex;
}

std::string ResourceManager::GetTextContent(const std::string& id) {
    if (!resourceTable) {
        LOG_ERROR("资源表未注册，无法加载 -> " << id);
        return "";
    }
    auto it = resourceTable->find(id);
    if (it == resourceTable->end()) {
        LOG_ERROR("找不到文本资源 -> " << id);
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
    LOG_INFO("ResourceManager 已清理所有纹理缓存");
}

const EmbeddedResource* ResourceManager::GetResource(const std::string& id) {
    if (!resourceTable) {
        LOG_ERROR("资源表未注册 -> " << id);
        return nullptr;
    }
    auto it = resourceTable->find(id);
    if (it == resourceTable->end()) {
        LOG_ERROR("找不到内嵌资源 -> " << id);
        return nullptr;
    }
    return &(it->second);
}

void ResourceManager::Draw(SDL_Texture* tex, SDL_Rect src, SDL_Rect dest, SDL_Renderer* ren, SDL_RendererFlip flip) {
    if (dest.w == 0 || dest.h == 0) {
        LOG_WARN("Destination width or height is 0!");
    }
    SDL_RenderCopyEx(ren, tex, &src, &dest, 0.0, nullptr, flip);
}

void ResourceManager::DrawWhole(SDL_Texture* tex, SDL_Rect dest, SDL_Renderer* ren, SDL_RendererFlip flip) {
    // 传 nullptr 给 src，SDL 会自动使用整张图
    SDL_RenderCopyEx(ren, tex, nullptr, &dest, 0.0, nullptr, flip);
}
