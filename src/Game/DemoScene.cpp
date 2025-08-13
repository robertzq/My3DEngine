#include "DemoScene.h"
#include "Engine/Engine.h"
#include "Engine/ShaderUtils.h"
#include <vector>
#include <cmath>
#include <cstdio>
#include <GLFW/glfw3.h>
#include <bx/math.h>   // 新增

struct PosVertex { float x, y, z; };

// --- 简单 4x4 矩阵工具（列主序，bgfx 兼容） ---
static void mtxIdentity(float m[16]) {
    for (int i=0;i<16;i++) m[i]=0.0f;
    m[0]=m[5]=m[10]=m[15]=1.0f;
}
static void mtxRotateY(float m[16], float rad) {
    mtxIdentity(m);
    float c = std::cos(rad), s = std::sin(rad);
    m[0]= c; m[2]= s;
    m[8]=-s; m[10]=c;
}
static void mtxTranslate(float m[16], float x, float y, float z) {
    mtxIdentity(m);
    m[12]=x; m[13]=y; m[14]=z;
}


static float s_view[16];
static float s_proj[16];
void DemoScene::setMvpColor(const float model[16], const float color[4]) {
     float vm[16], mvp[16];
       // bx::mtxMul(out, a, b) 表示 out = a * b  （列主序）
       bx::mtxMul(vm, s_view, model);   // V * M
       bx::mtxMul(mvp, s_proj, vm);     // P * (V*M)
       bgfx::setUniform(u_mvp,   mvp);
       bgfx::setUniform(u_color, color);
}

// --------- 几何构建：房间 ---------
void DemoScene::buildRoom() {
    const float S  = m_roomHalf; // xz [-S,S]
    const float Y0 = 0.0f;       // 地
    const float Y1 = 2.5f;       // 顶（现在先不画顶）

    std::vector<PosVertex> v;
    std::vector<uint16_t>  i;

    auto quad = [&](float x0,float y0,float z0, float x1,float y1,float z1,
                    float x2,float y2,float z2, float x3,float y3,float z3) {
        uint16_t base = (uint16_t)v.size();
        v.push_back({x0,y0,z0});
        v.push_back({x1,y1,z1});
        v.push_back({x2,y2,z2});
        v.push_back({x3,y3,z3});
        // 我们在盒子内部看，三角形需要“向内”。按下面顺序即可：
        i.push_back(base+0); i.push_back(base+1); i.push_back(base+2);
        i.push_back(base+0); i.push_back(base+2); i.push_back(base+3);
    };

    // 地面（y=Y0）
    quad(-S,Y0,-S,  S,Y0,-S,  S,Y0, S,  -S,Y0, S);

//    // 右墙 x=+S  面朝 -X
//    quad( S,Y0,-S,  S,Y1,-S,  S,Y1, S,  S,Y0, S);
//    // 左墙 x=-S  面朝 +X
//    quad(-S,Y0, S, -S,Y1, S, -S,Y1,-S, -S,Y0,-S);
//    // 前墙 z=+S  面朝 -Z
//    quad( S,Y0, S,  S,Y1, S, -S,Y1, S, -S,Y0, S);
//    // 后墙 z=-S  面朝 +Z
//    quad(-S,Y0,-S, -S,Y1,-S,  S,Y1,-S,  S,Y0,-S);

    const bgfx::Memory* vm = bgfx::copy(v.data(), (uint32_t)(v.size()*sizeof(PosVertex)));
    const bgfx::Memory* im = bgfx::copy(i.data(), (uint32_t)(i.size()*sizeof(uint16_t)));
    m_roomVbh = bgfx::createVertexBuffer(vm, m_layout);
    m_roomIbh = bgfx::createIndexBuffer(im);
}

// --------- 几何构建：球 ---------
void DemoScene::buildSphere(int stacks, int slices) {
    std::vector<PosVertex> v;
    std::vector<uint16_t>  idx;

    for (int y=0; y<=stacks; ++y) {
        float fy = float(y)/float(stacks);       // [0,1]
        float phi = fy * float(M_PI);            // [0,PI]
        float sy = std::cos(phi);
        float ry = std::sin(phi);

        for (int x=0; x<=slices; ++x) {
            float fx = float(x)/float(slices);   // [0,1]
            float th = fx * 2.0f * float(M_PI);
            float cx = std::cos(th);
            float sx = std::sin(th);
            float px = cx * ry;
            float py = sy;
            float pz = sx * ry;
            v.push_back({ px*m_ballR, py*m_ballR, pz*m_ballR });
        }
    }

    auto at = [&](int ix,int iy){ return (uint16_t)(iy*(slices+1)+ix); };

    for (int y=0; y<stacks; ++y) {
        for (int x=0; x<slices; ++x) {
            uint16_t i0 = at(x,   y);
            uint16_t i1 = at(x+1, y);
            uint16_t i2 = at(x+1, y+1);
            uint16_t i3 = at(x,   y+1);
            // 外表面
            idx.push_back(i0); idx.push_back(i1); idx.push_back(i2);
            idx.push_back(i0); idx.push_back(i2); idx.push_back(i3);
        }
    }

    const bgfx::Memory* vm = bgfx::copy(v.data(),   (uint32_t)(v.size()*sizeof(PosVertex)));
    const bgfx::Memory* im = bgfx::copy(idx.data(), (uint32_t)(idx.size()*sizeof(uint16_t)));
    m_ballVbh = bgfx::createVertexBuffer(vm, m_layout);
    m_ballIbh = bgfx::createIndexBuffer(im);
}

// ================= 生命周期 =================
void DemoScene::init(Engine* eng) {
    m_eng = eng;

    // --- 着色器 ---
    auto vsh = loadShaderFile("vs_simple.bin");
    auto fsh = loadShaderFile("fs_simple.bin");
    if (!bgfx::isValid(vsh) || !bgfx::isValid(fsh)) {
        std::fprintf(stderr, "[ERR] shader load failed\n");
        return;
    }
    m_prog = bgfx::createProgram(vsh, fsh, true);
    if (!bgfx::isValid(m_prog)) {
        std::fprintf(stderr, "[ERR] createProgram failed\n");
        return;
    }

    // --- uniforms ---
    u_mvp   = bgfx::createUniform("u_mvp",   bgfx::UniformType::Mat4);
    u_color = bgfx::createUniform("u_color", bgfx::UniformType::Vec4);

    // --- 顶点布局（仅位置） ---
    m_layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
    .end();

    // --- 构建几何 ---
    buildRoom();
    buildSphere(16, 24);

    m_lastTime = glfwGetTime();

    m_ready = bgfx::isValid(m_prog) && bgfx::isValid(u_mvp) && bgfx::isValid(u_color)
           && bgfx::isValid(m_roomVbh) && bgfx::isValid(m_roomIbh)
           && bgfx::isValid(m_ballVbh) && bgfx::isValid(m_ballIbh);

           // 相机
           const bx::Vec3 eye = { 0.0f, 1.5f, 6.0f };
           const bx::Vec3 at  = { 0.0f, 1.0f, 0.0f };
           const bx::Vec3 up  = { 0.0f, 1.0f, 0.0f };
           bx::mtxLookAt(s_view, eye, at, up);
           // 投影
           float aspect = 1280.0f/720.0f; // 先写死；更严谨可从 Engine 取窗口尺寸
           bx::mtxProj(s_proj, 60.0f, aspect, 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
}

void DemoScene::update() {
    if (!m_ready) return;

    double now = glfwGetTime();
    float dt = float(now - m_lastTime);
    if (dt > 0.05f) dt = 0.05f;  // 防卡顿穿墙
    m_lastTime = now;

    m_posX += m_velX * dt;
    m_posZ += m_velZ * dt;

    const float bound = m_roomHalf - m_ballR;
    if (m_posX >  bound) { m_posX =  bound; m_velX = -m_velX; }
    if (m_posX < -bound) { m_posX = -bound; m_velX = -m_velX; }
    if (m_posZ >  bound) { m_posZ =  bound; m_velZ = -m_velZ; }
    if (m_posZ < -bound) { m_posZ = -bound; m_velZ = -m_velZ; }

    float speed = std::sqrt(m_velX*m_velX + m_velZ*m_velZ);
    m_angle += (speed / m_ballR) * dt; // 简单双轮近似
}

void DemoScene::render() {
    if (!m_ready) return;

   // ===== 视图0：场景（开深度，不剔除）=====
   {
       // 1) 每帧用真实帧缓大小计算投影的宽高比
       const bgfx::Stats* st = bgfx::getStats();

       const uint16_t fbw = (st && st->width  > 0) ? (uint16_t)st->width  : 1280;
       const uint16_t fbh = (st && st->height > 0) ? (uint16_t)st->height : 720;
       bgfx::setViewRect(0, 0, 0, fbw, fbh);      // ★ 确保 View0 真在画
       bgfx::setViewTransform(0, s_view, s_proj); // 你已有
       bgfx::touch(0);
       const float aspect = (st && st->height > 0) ? float(st->width) / float(st->height) : (1280.0f/720.0f);

       // 2) 相机放在屋内稍远一些，保证能看见墙和地
       const bx::Vec3 eye = { 0.0f, 1.8f, 4.5f };
       const bx::Vec3 at  = { 0.0f, 0.6f, 0.0f };
       const bx::Vec3 up  = { 0.0f, 1.0f, 0.0f };
       bx::mtxLookAt(s_view, eye, at, up);
       bx::mtxProj (s_proj, 60.0f, aspect, 0.01f, 100.0f, bgfx::getCaps()->homogeneousDepth);
       bgfx::setViewTransform(0, s_view, s_proj);
       bgfx::touch(0);

       // --- 房间（深灰）：开深度写/测，但不剔除（我们在盒子内部看） ---
       {
           float M[16]; mtxIdentity(M);
           const float c[4] = { 0.6f, 0.6f, 0.6f, 1.0f };
           setMvpColor(M, c);

           bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                          BGFX_STATE_WRITE_Z   | BGFX_STATE_DEPTH_TEST_LESS /* 不剔除 */);

           bgfx::setVertexBuffer(0, m_roomVbh);
           bgfx::setIndexBuffer(m_roomIbh);
           bgfx::submit(0, m_prog);
       }

       // --- 小球（橙）：同样开深度 ---
       {
           float Ry[16], T[16], M[16];
           mtxRotateY(Ry, m_angle);
           mtxTranslate(T, m_posX, m_ballR, m_posZ);
           bx::mtxMul(M, T, Ry);

           const float c[4] = { 0.95f, 0.36f, 0.24f, 1.0f };
           setMvpColor(M, c);

           bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                          BGFX_STATE_WRITE_Z   | BGFX_STATE_DEPTH_TEST_LESS /* 不剔除 */);

           bgfx::setVertexBuffer(0, m_ballVbh);
           bgfx::setIndexBuffer(m_ballIbh);
           bgfx::submit(0, m_prog);
       }
   }

    // ===== 视图1：探针，保留（确认流水线没坏） =====
    {
        const uint16_t kProbeView = 1;
        float I[16]; for (int i=0;i<16;i++) I[i]=0; I[0]=I[5]=I[10]=I[15]=1.0f;
        bgfx::setViewRect(kProbeView, 16, 16, 240, 135); // 16:9 小角窗
        bgfx::setViewTransform(kProbeView, I, I);
        bgfx::touch(kProbeView);

        bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
        const float white[4] = {1,1,1,1};
        bgfx::setUniform(u_mvp, I);
        bgfx::setUniform(u_color, white);

        bgfx::TransientVertexBuffer tvb;
        if (bgfx::getAvailTransientVertexBuffer(3, m_layout) >= 3) {
            bgfx::allocTransientVertexBuffer(&tvb, 3, m_layout);
            struct V3{ float x,y,z; };
            V3* v = (V3*)tvb.data;
            v[0] = {-0.5f, -0.5f, 0.0f};
            v[1] = { 0.5f, -0.5f, 0.0f};
            v[2] = { 0.0f,  0.5f, 0.0f};
            bgfx::setVertexBuffer(0, &tvb);
            bgfx::submit(kProbeView, m_prog);
        }
    }
}






void DemoScene::shutdown() {
    if (bgfx::isValid(m_roomIbh)) { bgfx::destroy(m_roomIbh); m_roomIbh = BGFX_INVALID_HANDLE; }
    if (bgfx::isValid(m_roomVbh)) { bgfx::destroy(m_roomVbh); m_roomVbh = BGFX_INVALID_HANDLE; }
    if (bgfx::isValid(m_ballIbh)) { bgfx::destroy(m_ballIbh); m_ballIbh = BGFX_INVALID_HANDLE; }
    if (bgfx::isValid(m_ballVbh)) { bgfx::destroy(m_ballVbh); m_ballVbh = BGFX_INVALID_HANDLE; }
    if (bgfx::isValid(m_prog))    { bgfx::destroy(m_prog);    m_prog    = BGFX_INVALID_HANDLE; }
    if (bgfx::isValid(u_mvp))     { bgfx::destroy(u_mvp);     u_mvp     = BGFX_INVALID_HANDLE; }
    if (bgfx::isValid(u_color))   { bgfx::destroy(u_color);   u_color   = BGFX_INVALID_HANDLE; }
}
