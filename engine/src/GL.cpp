#include "Engine/GL.h"
#include "Engine/Log.h"
#include <SDL.h>

#define ENGINE_GL_DEFINE(ret, name, args) ret (*name) args = nullptr;
ENGINE_GL_FUNCTIONS(ENGINE_GL_DEFINE)
#undef ENGINE_GL_DEFINE

bool LoadGLFunctions() {
    bool ok = true;
#define ENGINE_GL_LOAD(ret, name, args)                                            \
    name = reinterpret_cast<ret(*)args>(SDL_GL_GetProcAddress(#name));             \
    if (!name) { LOG_ERROR("GL: 无法加载函数 " << #name); ok = false; }
    ENGINE_GL_FUNCTIONS(ENGINE_GL_LOAD)
#undef ENGINE_GL_LOAD
    return ok;
}
