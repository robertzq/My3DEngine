#include "Engine/Shader.h"
#include "Engine/GL.h"
#include "Engine/Log.h"
#include <vector>

namespace {

unsigned int CompileStage(GLenum type, const std::string& src, const std::string& name) {
    unsigned int shader = glCreateShader(type);
    const char* raw = src.c_str();
    glShaderSource(shader, 1, &raw, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len > 1 ? len : 1, '\0');
        glGetShaderInfoLog(shader, len, nullptr, log.data());
        LOG_ERROR("Shader 编译失败 [" << name << "] "
                  << (type == GL_VERTEX_SHADER ? "(vertex)" : "(fragment)") << ":\n" << log.data());
        LOG_ERROR("---- source ----\n" << src << "\n----------------");
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

}

Shader::~Shader() { Destroy(); }

bool Shader::LoadFromSource(const std::string& vertexSrc, const std::string& fragmentSrc,
                            const std::string& name) {
    Destroy();

    unsigned int vs = CompileStage(GL_VERTEX_SHADER, vertexSrc, name);
    if (!vs) return false;
    unsigned int fs = CompileStage(GL_FRAGMENT_SHADER, fragmentSrc, name);
    if (!fs) { glDeleteShader(vs); return false; }

    unsigned int program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len > 1 ? len : 1, '\0');
        glGetProgramInfoLog(program, len, nullptr, log.data());
        LOG_ERROR("Shader 链接失败 [" << name << "]:\n" << log.data());
        glDeleteProgram(program);
        return false;
    }

    program_ = program;
    uniformCache_.clear();
    return true;
}

void Shader::Destroy() {
    if (program_) {
        glDeleteProgram(program_);
        program_ = 0;
    }
    uniformCache_.clear();
}

void Shader::Use() const {
    if (program_) glUseProgram(program_);
}

int Shader::Uniform(const std::string& name) {
    auto it = uniformCache_.find(name);
    if (it != uniformCache_.end()) return it->second;
    // 缺失的 uniform 属于“可选”，静默跳过（不刷日志）
    int loc = glGetUniformLocation(program_, name.c_str());
    uniformCache_[name] = loc;
    return loc;
}

void Shader::SetInt(const std::string& name, int value) {
    int loc = Uniform(name);
    if (loc >= 0) glUniform1i(loc, value);
}

void Shader::SetFloat(const std::string& name, float value) {
    int loc = Uniform(name);
    if (loc >= 0) glUniform1f(loc, value);
}

void Shader::SetVec2(const std::string& name, float x, float y) {
    int loc = Uniform(name);
    if (loc >= 0) glUniform2f(loc, x, y);
}

void Shader::SetVec3(const std::string& name, float x, float y, float z) {
    int loc = Uniform(name);
    if (loc >= 0) glUniform3f(loc, x, y, z);
}

void Shader::SetVec4(const std::string& name, float x, float y, float z, float w) {
    int loc = Uniform(name);
    if (loc >= 0) glUniform4f(loc, x, y, z, w);
}
