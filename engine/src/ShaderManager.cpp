#include "Engine/ShaderManager.h"
#include "Engine/Log.h"
#include "Engine/ResourceManager.h"
#include "Engine/Shader.h"
#include <map>
#include <memory>

namespace {

std::map<std::string, std::unique_ptr<Shader>>& Cache() {
    static std::map<std::string, std::unique_ptr<Shader>> cache;
    return cache;
}

}

Shader* ShaderManager::Register(const std::string& id, const std::string& vertexSource,
                                const std::string& fragmentSource) {
    auto it = Cache().find(id);
    if (it != Cache().end()) {
        LOG_WARN("ShaderManager: shader id 已存在，返回已有实例 -> " << id);
        return it->second.get();
    }

    auto shader = std::make_unique<Shader>();
    if (!shader->LoadFromSource(vertexSource, fragmentSource, id)) {
        LOG_ERROR("ShaderManager: shader 编译/链接失败 -> " << id);
        return nullptr;
    }

    Shader* raw = shader.get();
    Cache()[id] = std::move(shader);
    LOG_INFO("ShaderManager: 已注册 shader -> " << id);
    return raw;
}

Shader* ShaderManager::Load(const std::string& id, const std::string& vertexResourceId,
                            const std::string& fragmentResourceId) {
    if (auto it = Cache().find(id); it != Cache().end()) return it->second.get();

    std::string vs = ResourceManager::GetText(vertexResourceId);
    std::string fs = ResourceManager::GetText(fragmentResourceId);
    if (vs.empty() || fs.empty()) {
        LOG_ERROR("ShaderManager: shader 资源缺失 -> " << id
                  << " (" << vertexResourceId << ", " << fragmentResourceId << ")");
        return nullptr;
    }
    return Register(id, vs, fs);
}

Shader* ShaderManager::Get(const std::string& id) {
    auto it = Cache().find(id);
    return it == Cache().end() ? nullptr : it->second.get();
}

void ShaderManager::Unload(const std::string& id) {
    Cache().erase(id);
}

void ShaderManager::Clean() {
    Cache().clear();
}
