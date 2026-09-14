#pragma once
#include <map>
#include <string>

// GLSL 程序：编译 / 链接 / uniform location 缓存。
class Shader {
public:
    ~Shader();

    bool LoadFromSource(const std::string& vertexSrc, const std::string& fragmentSrc,
                        const std::string& name);
    void Destroy();
    void Use() const;
    bool Valid() const { return program_ != 0; }

    void SetInt(const std::string& name, int value);
    void SetFloat(const std::string& name, float value);
    void SetVec2(const std::string& name, float x, float y);
    void SetVec3(const std::string& name, float x, float y, float z);
    void SetVec4(const std::string& name, float x, float y, float z, float w);

private:
    // 返回缓存过的 uniform location；不存在记 -1 并只警告一次
    int Uniform(const std::string& name);

    unsigned int program_ = 0;
    std::map<std::string, int> uniformCache_;
};
