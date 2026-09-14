#include "Engine/ResourceManager.h"
#include "Engine/Renderer.h"
#include "Engine/Log.h"
#include <SDL_image.h>
#include <fstream>

std::map<std::string, Texture*> ResourceManager::textureCache;
const ResourceTable* ResourceManager::resourceTable = nullptr;
std::string ResourceManager::basePath;
std::map<std::string, std::vector<unsigned char>> ResourceManager::fileBuffers;
std::map<std::string, EmbeddedResource> ResourceManager::fileResources;

void ResourceManager::Init() {
    size_t count = resourceTable ? resourceTable->size() : 0;
    LOG_INFO("ResourceManager 初始化完成。内嵌资源数量: " << count);
}

void ResourceManager::SetResourceTable(const ResourceTable& table) {
    resourceTable = &table;
}

void ResourceManager::SetBasePath(const std::string& path) {
    basePath = path;
}

bool ResourceManager::Has(const std::string& id) {
    if (resourceTable && resourceTable->find(id) != resourceTable->end()) return true;
    if (!basePath.empty()) {
        std::ifstream f(basePath + "/" + id, std::ios::binary);
        if (f.good()) return true;
    }
    return false;
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
    const EmbeddedResource* res = GetResource(id);
    if (!res) return nullptr;

    // 数据来自内嵌资源表或 File 来源，统一走内存流
    const unsigned char* data = res->data;
    size_t size = res->size;

    // 创建 SDL_RWops (内存流)，用 SDL_image 解码成 surface
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
    const EmbeddedResource* res = GetResource(id);
    if (!res) return "";
    // 二进制数据可能不包含 \0 结尾，必须指定长度
    return std::string(reinterpret_cast<const char*>(res->data), res->size);
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
    fileResources.clear();
    fileBuffers.clear();
    LOG_INFO("ResourceManager 已清理所有纹理缓存");
}

const EmbeddedResource* ResourceManager::GetResource(const std::string& id) {
    if (resourceTable) {
        auto it = resourceTable->find(id);
        if (it != resourceTable->end()) return &(it->second);
    }

    // File 来源 fallback（服务所有资源类型，不是 Audio 特例）
    if (!basePath.empty()) {
        auto cached = fileResources.find(id);
        if (cached != fileResources.end()) return &cached->second;

        std::ifstream f(basePath + "/" + id, std::ios::binary);
        if (f.good()) {
            std::vector<unsigned char> buf((std::istreambuf_iterator<char>(f)),
                                           std::istreambuf_iterator<char>());
            std::vector<unsigned char>& stored = fileBuffers[id];
            stored = std::move(buf);
            fileResources[id] = EmbeddedResource{stored.data(), stored.size()};
            LOG_INFO("ResourceManager: 从文件加载 -> " << id << " (" << stored.size() << " bytes)");
            return &fileResources[id];
        }
    }

    LOG_ERROR("找不到资源 -> " << id);
    return nullptr;
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
