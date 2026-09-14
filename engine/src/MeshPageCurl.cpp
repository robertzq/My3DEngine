#include "Engine/MeshPageCurl.h"
#include "Engine/GL.h"
#include "Engine/Log.h"
#include "Engine/Renderer.h"
#include "Engine/Shader.h"
#include "Engine/ShaderManager.h"
#include "Engine/Texture.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace {

// ---------- shaders ----------
const char* BG_VS =
    "#version 330 core\n"
    "layout(location=0) in vec2 aUnit; out vec2 vUv;\n"
    "void main(){ vUv=aUnit; gl_Position=vec4(aUnit*2.0-1.0,0.0,1.0); }\n";
const char* BG_FS =
    "#version 330 core\n"
    "in vec2 vUv; out vec4 F; uniform sampler2D u_inputTexture;\n"
    "uniform vec2 u_pageSize,uP0,uP1,uP2,uP3,u_dir;\n"
    "uniform float u_radius,u_shadow;\n"
    "vec2 bez(vec2 a,vec2 b,vec2 c,vec2 d,float t){ float m=1.0-t; return m*m*m*a+3.0*m*m*t*b+3.0*m*t*t*c+t*t*t*d; }\n"
    "vec2 bezT(vec2 a,vec2 b,vec2 c,vec2 d,float t){ float m=1.0-t; return 3.0*m*m*(b-a)+6.0*m*t*(c-b)+3.0*t*t*(d-c); }\n"
    "void main(){\n"
    "  vec2 suv=vec2(vUv.x,1.0-vUv.y); vec2 q=suv*u_pageSize;\n"
    "  float best=1e18,bt=0.0;\n"
    "  for(int i=0;i<=48;i++){ float t=float(i)/48.0; vec2 p=bez(uP0,uP1,uP2,uP3,t);\n"
    "    float d2=dot(p-q,p-q); if(d2<best){best=d2;bt=t;} }\n"
    "  vec2 C=bez(uP0,uP1,uP2,uP3,bt); vec2 T=normalize(bezT(uP0,uP1,uP2,uP3,bt)+vec2(1e-6));\n"
    "  vec2 N=normalize(vec2(-T.y,T.x)); if(dot(N,u_dir)<0.0) N=-N;\n"
    "  float d=dot(q-C,N);\n"
    "  vec3 col=texture(u_inputTexture,suv).rgb;\n"
    "  float sh=exp(-max(-d,0.0)/(u_radius*2.5))*u_shadow;\n"
    "  col*=(1.0-sh);\n"
    "  F=vec4(col,1.0);\n"
    "}\n";

const char* MESH_VS =
    "#version 330 core\n"
    "layout(location=0) in vec2 aPos;\n"
    "layout(location=1) in vec2 aUv;\n"
    "layout(location=2) in vec2 aExtra;\n"   // e0=curl/shadow, e1=region(0/1/2)
    "uniform vec2 u_resolution;\n"
    "uniform float u_radius;\n"
    "out vec2 vUv; out vec2 vExtra;\n"
    "void main(){\n"
    "  vUv=aUv; vExtra=aExtra;\n"
    "  float curl=(aExtra.y<0.5)?0.0:aExtra.x;\n"
    "  float z=u_radius*(1.0-cos(curl*3.14159265));\n"
    "  float depth=clamp(1.0 - z/(2.0*max(u_radius,1.0)),0.0,1.0);\n"
    "  float ndcX=(aPos.x/u_resolution.x)*2.0-1.0;\n"
    "  float ndcY=1.0-(aPos.y/u_resolution.y)*2.0;\n"
    "  gl_Position=vec4(ndcX,ndcY,depth*2.0-1.0,1.0);\n"
    "}\n";

const char* MESH_FS =
    "#version 330 core\n"
    "in vec2 vUv; in vec2 vExtra; out vec4 F;\n"
    "uniform sampler2D u_texture; uniform vec4 u_tint;\n"
    "uniform float u_shadow,u_highlight,u_backsideDarken;\n"
    "void main(){\n"
    "  if(vExtra.y<0.5){\n"                       // front
    "    vec3 rgb=texture(u_texture,vUv).rgb;\n"
    "    float shadow=u_shadow*(1.0-clamp(vExtra.x,0.0,1.0));\n"   // e0: 0 near fold -> strong
    "    rgb*=(1.0-shadow);\n"
    "    F=vec4(rgb,1.0)*u_tint;\n"
    "  } else {\n"
    "    float curl=(vExtra.y<1.5)?clamp(vExtra.x,0.0,1.0):1.0;\n"
    "    float back=smoothstep(0.5,0.9,curl);\n"
    "    vec2 uv=mix(vUv, vec2(1.0-vUv.x, vUv.y), back);\n"       // 页内镜像 backside
    "    vec3 rgb=texture(u_texture,uv).rgb;\n"
    "    rgb=mix(rgb, rgb*vec3(0.80,0.85,1.0)*(1.0-u_backsideDarken), back);\n"
    "    float hl=exp(-pow((curl-0.85)/0.13,2.0))*u_highlight;\n"
    "    float sh=(1.0-smoothstep(0.0,0.16,curl))*u_shadow*0.6;\n"
    "    rgb=rgb*(1.0-sh)+vec3(hl);\n"
    "    F=vec4(rgb,1.0)*u_tint;\n"
    "  }\n"
    "}\n";

// ---------- fold-local math ----------
struct Vec2 { float x, y; };
Vec2 Bez(const Vec2& a, const Vec2& b, const Vec2& c, const Vec2& d, float t) {
    float m = 1 - t;
    return {m * m * m * a.x + 3 * m * m * t * b.x + 3 * m * t * t * c.x + t * t * t * d.x,
            m * m * m * a.y + 3 * m * m * t * b.y + 3 * m * t * t * c.y + t * t * t * d.y};
}
Vec2 BezT(const Vec2& a, const Vec2& b, const Vec2& c, const Vec2& d, float t) {
    float m = 1 - t;
    return {3 * m * m * (b.x - a.x) + 6 * m * t * (c.x - b.x) + 3 * t * t * (d.x - c.x),
            3 * m * m * (b.y - a.y) + 6 * m * t * (c.y - b.y) + 3 * t * t * (d.y - c.y)};
}
struct Fold { Vec2 Q, N; float d; };
Fold Closest(const Vec2& a, const Vec2& b, const Vec2& c, const Vec2& d, const Vec2& dir, const Vec2& q) {
    const int N = 64; float best = 1e18f, bt = 0.0f;
    for (int i = 0; i <= N; ++i) {
        float t = (float)i / N; Vec2 p = Bez(a, b, c, d, t);
        float e = (p.x - q.x) * (p.x - q.x) + (p.y - q.y) * (p.y - q.y);
        if (e < best) { best = e; bt = t; }
    }
    float step = 1.0f / N;
    for (int k = 0; k < 6; ++k) {
        float t0 = std::max(bt - step, 0.0f), t1 = std::min(bt + step, 1.0f);
        Vec2 pa = Bez(a, b, c, d, t0), pb = Bez(a, b, c, d, t1);
        float da = (pa.x - q.x) * (pa.x - q.x) + (pa.y - q.y) * (pa.y - q.y);
        float db = (pb.x - q.x) * (pb.x - q.x) + (pb.y - q.y) * (pb.y - q.y);
        if (da < best) { best = da; bt = t0; }
        if (db < best) { best = db; bt = t1; }
        step *= 0.5f;
    }
    Vec2 Q = Bez(a, b, c, d, bt);
    Vec2 t = BezT(a, b, c, d, bt);
    float len = std::sqrt(t.x * t.x + t.y * t.y); if (len < 1e-6f) len = 1.0f;
    Vec2 Nn{t.x / len, t.y / len}; Nn = {-Nn.y, Nn.x};
    if (Nn.x * dir.x + Nn.y * dir.y < 0.0f) { Nn.x = -Nn.x; Nn.y = -Nn.y; }
    float dd = (q.x - Q.x) * Nn.x + (q.y - Q.y) * Nn.y;
    return {Q, Nn, dd};
}

struct PV { float x, y, u, v, d; int idx; };
PV LerpPV(const PV& a, const PV& b, float t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.u + (b.u - a.u) * t,
            a.v + (b.v - a.v) * t, a.d + (b.d - a.d) * t, -1};
}
void ClipPoly(const std::vector<PV>& in, float thr, bool keepGreater, std::vector<PV>& out) {
    out.clear();
    int n = (int)in.size(); if (n == 0) return;
    for (int i = 0; i < n; ++i) {
        const PV& cur = in[i]; const PV& nxt = in[(i + 1) % n];
        bool ci = keepGreater ? cur.d >= thr : cur.d < thr;
        bool ni = keepGreater ? nxt.d >= thr : nxt.d < thr;
        if (ci) out.push_back(cur);
        if (ci != ni) out.push_back(LerpPV(cur, nxt, (thr - cur.d) / (nxt.d - cur.d)));
    }
}

bool g_inited = false;

} // namespace

bool MeshPageCurl::Init() {
    if (!ShaderManager::Register("page_curl_bg", BG_VS, BG_FS)) return false;
    if (!ShaderManager::Register("page_curl_mesh", MESH_VS, MESH_FS)) return false;
    g_inited = true;
    return true;
}
void MeshPageCurl::Clean() {}

void MeshPageCurl::Render(const Texture* pageTexture, const Texture* backTexture,
                          int width, int height, const MeshPageCurlParams& params) {
    if (!g_inited || width <= 0 || height <= 0 || !pageTexture) return;

    const float W = (float)width, H = (float)height;
    const float minDim = std::min(W, H);
    const float R = std::max(1.0f, params.curlRadius * minDim);
    const float PI = 3.14159265f;
    const float arcLen = PI * R;

    float dx = 1.0f - 2.0f * params.originX, dy = 1.0f - 2.0f * params.originY;
    float dl = std::sqrt(dx * dx + dy * dy);
    if (dl < 1e-4f) { dx = -1.0f; dy = -1.0f; dl = std::sqrt(2.0f); }
    dx /= dl; dy /= dl;
    const float perpX = -dy, perpY = dx;
    const float diag = std::sqrt(W * W + H * H);

    float sp;
    if (params.progress < 0.2f) sp = 0.15f * (params.progress / 0.2f) * (params.progress / 0.2f);
    else if (params.progress < 0.8f) sp = 0.15f + 0.70f * ((params.progress - 0.2f) / 0.6f);
    else sp = 0.85f + 0.15f * ((params.progress - 0.8f) / 0.2f);
    sp = std::min(std::max(sp, 0.0f), 1.0f);

    Vec2 base{params.originX * W + dx * sp * diag, params.originY * H + dy * sp * diag};
    if (params.dragX != params.originX || params.dragY != params.originY) {
        base.x = base.x * 0.7f + params.dragX * W * 0.3f;
        base.y = base.y * 0.7f + params.dragY * H * 0.3f;
    }
    const float span = W + H;
    Vec2 P0{base.x - perpX * span, base.y - perpY * span};
    Vec2 P3{base.x + perpX * span, base.y + perpY * span};
    Vec2 P1, P2;
    if (params.explicitControl) {
        P1 = {params.control1X * W, params.control1Y * H};
        P2 = {params.control2X * W, params.control2Y * H};
    } else {
        float bow = params.curvature * span * 0.22f;
        P1 = {P0.x + dx * bow, P0.y + dy * bow};
        P2 = {P3.x + dx * bow, P3.y + dy * bow};
    }
    Vec2 dir{dx, dy};

    // grid
    int N = std::max(8, std::min(params.tessellation, 96));
    int gw = N + 1;
    std::vector<float> dgrid(gw * gw);
    for (int j = 0; j <= N; ++j)
        for (int i = 0; i <= N; ++i) {
            Vec2 q{i * W / N, j * H / N};
            dgrid[j * gw + i] = Closest(P0, P1, P2, P3, dir, q).d;
        }

    static std::vector<MeshVertex> verts;
    static std::vector<unsigned short> indices;
    verts.clear();
    indices.clear();

    // helper: push a polygon (list of PV) belonging to region (0 front,1 curl,2 flap)
    auto emit = [&](const std::vector<PV>& poly, int region) {
        if (poly.size() < 3) return;
        unsigned short base = (unsigned short)verts.size();
        for (const PV& pv : poly) {
            MeshVertex v;
            v.u = pv.u; v.v = pv.v;
            if (region == 0) {
                v.x = pv.x; v.y = pv.y;
                v.e0 = std::min(std::max(pv.d / (R * 4.0f), 0.0f), 1.0f);
                v.e1 = 0.0f;
            } else {
                Fold f = Closest(P0, P1, P2, P3, dir, Vec2{pv.x, pv.y});
                float s = -f.d;
                if (region == 1) {
                    float phi = std::min(std::max(s / R, 0.0f), PI);
                    float off = -R * std::sin(phi);
                    v.x = f.Q.x + f.N.x * off; v.y = f.Q.y + f.N.y * off;
                    v.e0 = phi / PI; v.e1 = 1.0f;
                } else {
                    float off = s - arcLen;
                    v.x = f.Q.x + f.N.x * off; v.y = f.Q.y + f.N.y * off;
                    v.e0 = 1.0f; v.e1 = 2.0f;
                }
            }
            verts.push_back(v);
        }
        for (unsigned short k = 1; k + 1 < (unsigned short)poly.size(); ++k) {
            indices.push_back(base);
            indices.push_back((unsigned short)(base + k));
            indices.push_back((unsigned short)(base + k + 1));
        }
    };

    std::vector<PV> quad(4), frontPoly, turned, curlPoly, flapPoly;
    for (int j = 0; j < N; ++j)
        for (int i = 0; i < N; ++i) {
            int c00 = j * gw + i, c10 = c00 + 1, c01 = c00 + gw, c11 = c01 + 1;
            auto mk = [&](int idx) {
                int ci = idx % gw, cj = idx / gw;
                return PV{ci * W / N, cj * H / N, (float)ci / N, (float)cj / N, dgrid[idx], -1};
            };
            quad[0] = mk(c00); quad[1] = mk(c10); quad[2] = mk(c11); quad[3] = mk(c01);
            ClipPoly(quad, 0.0f, true, frontPoly);
            ClipPoly(quad, 0.0f, false, turned);
            ClipPoly(turned, -arcLen, true, curlPoly);
            ClipPoly(turned, -arcLen, false, flapPoly);
            emit(frontPoly, 0);
            emit(curlPoly, 1);
            emit(flapPoly, 2);
        }

    if (indices.empty()) return;

    // background (new scene) + fold shadow; no depth write
    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    Shader* bg = ShaderManager::Get("page_curl_bg");
    if (bg && backTexture) {
        bg->Use();
        bg->SetVec2("u_pageSize", W, H);
        bg->SetVec2("uP0", P0.x, P0.y); bg->SetVec2("uP1", P1.x, P1.y);
        bg->SetVec2("uP2", P2.x, P2.y); bg->SetVec2("uP3", P3.x, P3.y);
        bg->SetVec2("u_dir", dx, dy);
        bg->SetFloat("u_radius", R);
        bg->SetFloat("u_shadow", params.shadowStrength);
        Renderer::DrawFullscreen(*bg, backTexture);
    }

    // page mesh with depth ordering (curl/flap above front)
    Shader* sh = ShaderManager::Get("page_curl_mesh");
    if (!sh || !sh->Valid()) return;
    sh->Use();
    sh->SetFloat("u_radius", R);
    sh->SetFloat("u_shadow", params.shadowStrength);
    sh->SetFloat("u_highlight", params.highlightStrength);
    sh->SetFloat("u_backsideDarken", params.backsideDarken);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    Renderer::DrawMesh(*sh, verts.data(), (int)verts.size(), indices.data(), (int)indices.size(), pageTexture);
    glDisable(GL_DEPTH_TEST);
    Renderer::CheckError("page curl mesh");
}
