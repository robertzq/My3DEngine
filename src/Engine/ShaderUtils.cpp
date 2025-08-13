#include "Engine/ShaderUtils.h"
#include <bgfx/bgfx.h>

#include <cstdio>
#include <cerrno>
#include <cstring>
#include <vector>
#include <string>
#include <fstream>
#include <sys/stat.h>

#if __APPLE__
  #include <mach-o/dyld.h>
  #include <unistd.h>
#endif

// 获取可执行文件所在目录
static std::string exeDir() {
    char buf[4096];
    uint32_t size = sizeof(buf);
#if __APPLE__
    if (_NSGetExecutablePath(buf, &size) == 0) {
        char real[4096];
        if (::realpath(buf, real)) {
            std::string p(real);
            auto pos = p.find_last_of('/');
            return (pos==std::string::npos) ? "." : p.substr(0, pos);
        }
    }
#endif
    return ".";
}

static bool fileExists(const std::string& p) {
    struct stat st;
    return ::stat(p.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

static bgfx::ShaderHandle loadRaw(const std::string& path) {
    // 先用 C 风格尝试，便于拿到 errno
    FILE* fp = ::fopen(path.c_str(), "rb");
    if (!fp) {
        std::fprintf(stderr, "[ERR] fopen failed: %s (%s)\n",
                     path.c_str(), std::strerror(errno));
        return BGFX_INVALID_HANDLE;
    }
    std::vector<uint8_t> data;
    ::fseek(fp, 0, SEEK_END);
    long sz = ::ftell(fp);
    if (sz <= 0) { ::fclose(fp); return BGFX_INVALID_HANDLE; }
    ::fseek(fp, 0, SEEK_SET);
    data.resize((size_t)sz);
    size_t rd = ::fread(data.data(), 1, (size_t)sz, fp);
    ::fclose(fp);
    if (rd != (size_t)sz) {
        std::fprintf(stderr, "[ERR] fread short: %s (read=%zu, sz=%ld)\n",
                     path.c_str(), rd, sz);
        return BGFX_INVALID_HANDLE;
    }

    const bgfx::Memory* mem = bgfx::copy(data.data(), (uint32_t)data.size());
    return bgfx::createShader(mem);
}

bgfx::ShaderHandle loadShaderFile(const char* rel) {
    const std::string base = exeDir();

    // 我们尝试四个路径，逐个打印：
    const std::string p1 = base + "/assets/shaders_bin/" + rel;   // 你 CMake 的输出目录
    const std::string p2 = base + "/assets/shaders/"     + rel;   // 兜底
    const std::string p3 = base + "/" + rel;                      // 同目录兜底
    const std::string p4 = std::string(rel);                      // 纯相对（以防 run 脚本改了 cwd）

    const char* tryList[4] = { p1.c_str(), p2.c_str(), p3.c_str(), p4.c_str() };

    for (int i = 0; i < 4; ++i) {
        std::fprintf(stderr, "[dbg] try shader: %s\n", tryList[i]);
        if (!fileExists(tryList[i])) {
            std::fprintf(stderr, "[dbg] not exists: %s\n", tryList[i]);
            continue;
        }
        bgfx::ShaderHandle h = loadRaw(tryList[i]);
        if (bgfx::isValid(h)) {
            std::fprintf(stderr, "[dbg] loaded shader: %s (size OK)\n", tryList[i]);
            return h;
        } else {
            std::fprintf(stderr, "[dbg] createShader failed on: %s\n", tryList[i]);
        }
    }

    std::fprintf(stderr, "[ERR] cannot open shader after tries: %s | %s | %s | %s\n",
                 p1.c_str(), p2.c_str(), p3.c_str(), p4.c_str());
    return BGFX_INVALID_HANDLE;
}
