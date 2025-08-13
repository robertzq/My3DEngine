#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <bgfx/bgfx.h>
#include "Engine/Renderer/BasicPipeline.h"

struct Mesh {
    bgfx::VertexBufferHandle vbh = BGFX_INVALID_HANDLE;
    bgfx::IndexBufferHandle ibh = BGFX_INVALID_HANDLE;
    uint32_t indexCount = 0;

    void submit(uint8_t viewId, bgfx::ProgramHandle program) const {
        if (!bgfx::isValid(program) || !bgfx::isValid(vbh) || !bgfx::isValid(ibh) || indexCount == 0) {
            return;
        }
        bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z
                    | BGFX_STATE_DEPTH_TEST_LESS /*| BGFX_STATE_CULL_CW*/);
        bgfx::setVertexBuffer(0, vbh);
        bgfx::setIndexBuffer(ibh, 0, indexCount);
        bgfx::submit(viewId, program);
    }

    void destroy() {
        if (bgfx::isValid(vbh)) bgfx::destroy(vbh);
        if (bgfx::isValid(ibh)) bgfx::destroy(ibh);
    }
};

Mesh makePlane(float halfSize);
Mesh makeWall(float halfSize, float height);
Mesh makeIcosphere(int subdiv, float radius);

