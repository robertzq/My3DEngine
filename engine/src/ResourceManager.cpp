#include "Engine/ResourceManager.h"
#include "Engine/Renderer.h"
#include "Engine/Log.h"
#include <SDL_image.h>

std::map<std::string, Texture*> ResourceManager::textureCache;
const ResourceTable* ResourceManager::resourceTable = nullptr;

void ResourceManager::Init() {
    size_t count = resourceTable ? resourceTable->size() : 0;
    LOG_INFO("ResourceManager 初始化完成。内嵌资源数量: " << count);
}

void ResourceManager::SetResourceTable(const ResourceTable& table) {
    resourceTable = &table;
}

bool ResourceManager::Has(const std::string& id) {
    return resourceTable && resourceTable->find(id) != resourceTable->end();
}

Texture* ResourceManager::GetTexture(const std::string& id) {
    // 1. 先查缓存
    auto it = textureCache.find(id);
    if (it != textureCache.end()) {
        return it->second;
    }

    // 2. 缓存没有，尝试加载
    Texture* tex = LoadTextureFromMemory(id);
    if (tex) {
        textureCache[id] = tex;
    }
    return tex;
}

Texture* ResourceManager::LoadTextureFromMemory(const std::string& id) {
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

    // 3. 创建 SDL_RWops (内存流)，用 SDL_image 解码成 surface
    SDL_RWops* rw = SDL_RWFromConstMem(data, (int)size);
    if (!rw) {
        LOG_ERROR("SDL_RWFromConstMem 失败: " << SDL_GetError());
        return nullptr;
    }

    SDL_Surface* surface = IMG_Load_RW(rw, 1);   // freesrc=1
    if (!surface) {
        LOG_ERROR("IMG_Load_RW 失败 (" << id << "): " << IMG_GetError());
        return nullptr;
    }

    // 4. 上传成 GL 纹理（所有权转交 ResourceManager）
    Texture* tex = Renderer::CreateTextureFromSurface(surface);
    SDL_FreeSurface(surface);
    return tex;
}

std::string ResourceManager::GetText(const std::string& id) {
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

void ResourceManager::Unload(const std::string& id) {
    auto it = textureCache.find(id);
    if (it == textureCache.end()) return;
    Renderer::DestroyTexture(it->second);
    textureCache.erase(it);
}

void ResourceManager::Clear() {
    for (auto& pair : textureCache) {
        Renderer::DestroyTexture(pair.second);
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

void ResourceManager::Draw(Texture* tex, SDL_Rect src, SDL_Rect dest, SDL_RendererFlip flip) {
    if (dest.w == 0 || dest.h == 0) {
        LOG_WARN("Destination width or height is 0!");
    }
    Renderer::DrawSprite(tex, src, dest, flip);
}

void ResourceManager::DrawWhole(Texture* tex, SDL_Rect dest, SDL_RendererFlip flip) {
    Renderer::DrawSprite(tex, {0, 0, 0, 0}, dest, flip);   // src 宽高<=0 => 整张纹理
}
