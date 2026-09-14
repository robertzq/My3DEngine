#pragma once
#include <string>

class Shader;

// 引擎级 shader 缓存：由 cache 持有 Shader 所有权，其它地方只持 non-owning Shader*。
//
// * 同 id 不重复 compile/link（Register 已存在则返回已有实例）。
// * source 可来自内嵌资源：Load(id, vertexResourceId, fragmentResourceId) 经 ResourceManager::GetText。
// * ShaderManager::Clean() 必须在 GL context 销毁之前调用（Game::clean 已保证）。
// * 不引入 shared_ptr / refcount；不放进 ResourceManager 的 texture cache。
class ShaderManager {
public:
    // 用源码注册；id 已存在则返回已有实例（并记录一次警告）
    static Shader* Register(const std::string& id, const std::string& vertexSource,
                            const std::string& fragmentSource);

    // 用资源 id 的文本作为源码注册（不再单独引入 ResourceProvider）
    static Shader* Load(const std::string& id, const std::string& vertexResourceId,
                        const std::string& fragmentResourceId);

    static Shader* Get(const std::string& id);
    static void Unload(const std::string& id);
    static void Clean();
};
