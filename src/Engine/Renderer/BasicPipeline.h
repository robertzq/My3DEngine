#pragma once
#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

struct PosNormVertex {
    float x,y,z;
    float nx,ny,nz;
    static void initLayout(bgfx::VertexLayout& layout) {
        layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Normal,   3, bgfx::AttribType::Float)
            .end();
    }
};

struct BasicPipeline {
    bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_mvp = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_normalMat = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_color = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_lightDir = BGFX_INVALID_HANDLE;

    bool load(const char* vsBinPath, const char* fsBinPath);
    void destroy();
};

