#include "BasicPipeline.h"
#include <cstdio>
#include <vector>
#include <fstream>
#include <string>

#if __APPLE__
  #include <mach-o/dyld.h>   // _NSGetExecutablePath
  #include <unistd.h>        // realpath
#elif _WIN32
  #include <windows.h>       // GetModuleFileNameA
#else
  #include <unistd.h>        // readlink
  #include <limits.h>        // PATH_MAX
#endif

static std::string exeDir() {
#if __APPLE__
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);     // 先拿所需长度
    std::string buf(size, '\0');
    if (_NSGetExecutablePath(buf.data(), &size) == 0) {
        char real[4096] = {0};
        if (::realpath(buf.c_str(), real)) {
            std::string p(real);
            auto pos = p.find_last_of('/');
            return (pos == std::string::npos) ? "." : p.substr(0, pos);
        }
    }
    return ".";
#elif _WIN32
    char path[MAX_PATH] = {0};
    DWORD len = GetModuleFileNameA(nullptr, path, MAX_PATH);
    std::string p(path, (len ? len : 0));
    auto pos = p.find_last_of("\\/");
    return (pos == std::string::npos) ? "." : p.substr(0, pos);
#else
    char path[PATH_MAX] = {0};
    ssize_t len = ::readlink("/proc/self/exe", path, sizeof(path)-1);
    if (len > 0) {
        path[len] = '\0';
        std::string p(path);
        auto pos = p.find_last_of('/');
        return (pos == std::string::npos) ? "." : p.substr(0, pos);
    }
    return ".";
#endif
}

static bgfx::ShaderHandle loadShader(const char* rel) {
    // 先找编译产物目录，再回退到旧目录
    std::string base = exeDir() + "/assets/";
    std::string path1 = base + "shaders_bin/" + rel; // 新目录
    std::string path2 = base + "shaders/"     + rel; // 旧目录（兜底）

    auto tryLoad = [](const std::string& p) -> bgfx::ShaderHandle {
        std::ifstream f(p, std::ios::binary);
        if (!f) return BGFX_INVALID_HANDLE;
        std::vector<char> data((std::istreambuf_iterator<char>(f)), {});
        if (data.empty()) return BGFX_INVALID_HANDLE;
        const bgfx::Memory* mem = bgfx::copy(data.data(), (uint32_t)data.size());
        return bgfx::createShader(mem);
    };

    auto h = tryLoad(path1);
    if (!bgfx::isValid(h)) {
        fprintf(stderr, "[WARN] shader not in shaders_bin, try shaders: %s\n", path2.c_str());
        h = tryLoad(path2);
    }
    if (!bgfx::isValid(h)) {
        fprintf(stderr, "[ERR] Cannot open shader: %s or %s\n", path1.c_str(), path2.c_str());
    }
    return h;
}
bgfx::ShaderHandle loadShaderFileBP(const char* rel) {
    std::string path = exeDir() + "/assets/shaders_bin/" + rel;
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        fprintf(stderr, "[ERR] Cannot open shader file: %s\n", path.c_str());
        return BGFX_INVALID_HANDLE;
    }
    std::vector<char> data((std::istreambuf_iterator<char>(f)), {});
    const bgfx::Memory* mem = bgfx::copy(data.data(), (uint32_t)data.size());
    return bgfx::createShader(mem);
}

bool BasicPipeline::load(const char* vsBin, const char* fsBin) {
    auto vsh = loadShader(vsBin);
    auto fsh = loadShader(fsBin);
    if (!bgfx::isValid(vsh) || !bgfx::isValid(fsh)) {
        fprintf(stderr, "[ERR] Invalid shader handle(s). vs:%d fs:%d\n",
                bgfx::isValid(vsh), bgfx::isValid(fsh));
        return false;
    }
    program = bgfx::createProgram(vsh, fsh, true);
    if (!bgfx::isValid(program)) {
        fprintf(stderr, "[ERR] createProgram failed.\n");
        return false;
    }
    u_mvp       = bgfx::createUniform("u_mvp", bgfx::UniformType::Mat4);
    u_normalMat = bgfx::createUniform("u_normalMat", bgfx::UniformType::Mat3);
    u_color     = bgfx::createUniform("u_color", bgfx::UniformType::Vec4);
    u_lightDir  = bgfx::createUniform("u_lightDir", bgfx::UniformType::Vec4);
    return true;
}


void BasicPipeline::destroy() {
    if (bgfx::isValid(program)) bgfx::destroy(program);
    if (bgfx::isValid(u_mvp)) bgfx::destroy(u_mvp);
    if (bgfx::isValid(u_normalMat)) bgfx::destroy(u_normalMat);
    if (bgfx::isValid(u_color)) bgfx::destroy(u_color);
    if (bgfx::isValid(u_lightDir)) bgfx::destroy(u_lightDir);
}

