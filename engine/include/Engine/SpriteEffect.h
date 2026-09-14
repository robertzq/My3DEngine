#pragma once
#include <map>
#include <string>

class Shader;

// 轻量 sprite effect 实例：持 non-owning Shader* + 自己的参数。
// 同一 shader 可被多个 SpriteEffect 使用，参数互不影响。
// 不做 Material / node graph。
class SpriteEffect {
public:
    SpriteEffect() = default;
    explicit SpriteEffect(Shader* shader) : shader_(shader) {}

    void SetShader(Shader* shader) { shader_ = shader; }
    Shader* GetShader() const { return shader_; }

    void SetInt(const std::string& n, int value) { Param p; p.kind = 1; p.i = value; params_[n] = p; }
    void SetFloat(const std::string& n, float value) { Param p; p.kind = 2; p.f[0] = value; params_[n] = p; }
    void SetVec2(const std::string& n, float x, float y) { Param p; p.kind = 3; p.f[0] = x; p.f[1] = y; params_[n] = p; }
    void SetVec3(const std::string& n, float x, float y, float z) { Param p; p.kind = 4; p.f[0] = x; p.f[1] = y; p.f[2] = z; params_[n] = p; }
    void SetVec4(const std::string& n, float x, float y, float z, float w) { Param p; p.kind = 5; p.f[0] = x; p.f[1] = y; p.f[2] = z; p.f[3] = w; params_[n] = p; }

    // kind: 1=int 2=float 3=vec2 4=vec3 5=vec4
    struct Param { int kind = 0; int i = 0; float f[4] = {0, 0, 0, 0}; };
    const std::map<std::string, Param>& Params() const { return params_; }

private:
    Shader* shader_ = nullptr;
    std::map<std::string, Param> params_;
};
