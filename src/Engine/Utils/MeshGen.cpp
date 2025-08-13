#include "MeshGen.h"
#include <bgfx/bgfx.h>
#include <vector>
#include <cstring>

static bgfx::VertexLayout s_layout;
static bool s_layoutInit = false;

struct V { float x,y,z, nx,ny,nz; };

static void ensureLayout() {
    if (s_layoutInit) return;
    PosNormVertex::initLayout(s_layout);
    s_layoutInit = true;
}

static Mesh makeFrom(const std::vector<V>& verts, const std::vector<uint16_t>& idx) {
    ensureLayout();
    const bgfx::Memory* vmem = bgfx::copy(verts.data(), (uint32_t)(verts.size()*sizeof(V)));
    const bgfx::Memory* imem = bgfx::copy(idx.data(),   (uint32_t)(idx.size()*sizeof(uint16_t)));
    Mesh m;
    m.vbh = bgfx::createVertexBuffer(vmem, s_layout);
    m.ibh = bgfx::createIndexBuffer(imem);
    m.indexCount = (uint32_t)idx.size();
    return m;
}

Mesh makePlane(float half) {
    std::vector<V> v = {
        {-half,0,-half, 0,1,0},
        { half,0,-half, 0,1,0},
        { half,0, half, 0,1,0},
        {-half,0, half, 0,1,0},
    };
    std::vector<uint16_t> i = {0,1,2, 0,2,3};
    return makeFrom(v,i);
}

Mesh makeWall(float half, float h) {
    // 竖着的一张“墙”朝内法线，默认位于XZ平面边缘时再通过模型矩阵旋转到位
    std::vector<V> v = {
        {-half, 0, 0,   0,0,1},
        {-half, h, 0,   0,0,1},
        { half, h, 0,   0,0,1},
        { half, 0, 0,   0,0,1},
    };
    std::vector<uint16_t> i = {0,1,2, 0,2,3};
    return makeFrom(v,i);
}

Mesh makeIcosphere(int /*subdiv*/, float r) {
    // 先用“小立方体”占位，保证能跑（之后再换成真正球）
    const float s = r;
    std::vector<V> v = {
        // +X
        { s,-s,-s, 1,0,0}, { s, s,-s, 1,0,0}, { s, s, s, 1,0,0}, { s,-s, s, 1,0,0},
        // -X
        {-s,-s, s,-1,0,0}, {-s, s, s,-1,0,0}, {-s, s,-s,-1,0,0}, {-s,-s,-s,-1,0,0},
        // +Y
        {-s, s,-s, 0,1,0}, {-s, s, s, 0,1,0}, { s, s, s, 0,1,0}, { s, s,-s, 0,1,0},
        // -Y
        {-s,-s, s, 0,-1,0}, {-s,-s,-s, 0,-1,0}, { s,-s,-s, 0,-1,0}, { s,-s, s, 0,-1,0},
        // +Z
        {-s,-s, s, 0,0,1}, { s,-s, s, 0,0,1}, { s, s, s, 0,0,1}, {-s, s, s, 0,0,1},
        // -Z
        { s,-s,-s, 0,0,-1}, {-s,-s,-s, 0,0,-1}, {-s, s,-s, 0,0,-1}, { s, s,-s, 0,0,-1},
    };
    std::vector<uint16_t> i = {
        0,1,2, 0,2,3,  4,5,6, 4,6,7,
        8,9,10, 8,10,11, 12,13,14, 12,14,15,
        16,17,18, 16,18,19, 20,21,22, 20,22,23
    };
    return makeFrom(v,i);
}

