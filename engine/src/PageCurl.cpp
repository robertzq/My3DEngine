#include "Engine/PageCurl.h"
#include "Engine/GL.h"
#include "Engine/Log.h"
#include "Engine/Renderer.h"
#include "Engine/Shader.h"
#include "Engine/ShaderManager.h"
#include "Engine/Texture.h"
#include <algorithm>
#include <cmath>

namespace {

const char* CURL_VS =
    "#version 330 core\n"
    "layout(location=0) in vec2 aUnit;\n"
    "out vec2 vUv;\n"
    "void main(){ vUv = aUnit; gl_Position = vec4(aUnit*2.0-1.0, 0.0, 1.0); }\n";

const char* CURL_FS =
    "#version 330 core\n"
    "in vec2 vUv; out vec4 F;\n"
    "uniform sampler2D u_inputTexture;\n"   // page (old)
    "uniform sampler2D u_back;\n"            // revealed (new)
    "uniform vec2 u_pageSize;\n"
    "uniform vec2 uP0, uP1, uP2, uP3;\n"
    "uniform vec2 u_dir;\n"
    "uniform float u_radius, u_curvature;\n"
    "uniform float u_shadow, u_highlight, u_backsideDarken, u_edgeSoft;\n"
    "uniform float u_progress;\n"
    "vec2 bez(vec2 p0,vec2 p1,vec2 p2,vec2 p3,float t){ float m=1.0-t;\n"
    "  return m*m*m*p0 + 3.0*m*m*t*p1 + 3.0*m*t*t*p2 + t*t*t*p3; }\n"
    "vec2 bezT(vec2 p0,vec2 p1,vec2 p2,vec2 p3,float t){ float m=1.0-t;\n"
    "  return 3.0*m*m*(p1-p0) + 6.0*m*t*(p2-p1) + 3.0*t*t*(p3-p2); }\n"
    "void closest(vec2 q, out vec2 C, out vec2 T, out float bt){\n"
    "  const int N=48; float best=1e18; bt=0.0;\n"
    "  for(int i=0;i<=N;i++){ float t=float(i)/float(N); vec2 p=bez(uP0,uP1,uP2,uP3,t);\n"
    "    float d2=dot(p-q,p-q); if(d2<best){best=d2;bt=t;} }\n"
    "  float step=1.0/float(N);\n"
    "  for(int k=0;k<6;k++){ float t0=max(bt-step,0.0), t1=min(bt+step,1.0);\n"
    "    vec2 pa=bez(uP0,uP1,uP2,uP3,t0), pb=bez(uP0,uP1,uP2,uP3,t1);\n"
    "    float da=dot(pa-q,pa-q), db=dot(pb-q,pb-q);\n"
    "    if(da<best){best=da;bt=t0;} if(db<best){best=db;bt=t1;} step*=0.5; }\n"
    "  C=bez(uP0,uP1,uP2,uP3,bt); T=normalize(bezT(uP0,uP1,uP2,uP3,bt)+vec2(1e-6));\n"
    "}\n"
    "void main(){\n"
    "  vec2 suv=vec2(vUv.x, 1.0-vUv.y);\n"
    "  vec2 q=suv*u_pageSize;\n"
    "  vec2 C,T; float bt; closest(q,C,T,bt);\n"
    "  vec2 N=normalize(vec2(-T.y,T.x));\n"
    "  if(dot(N,u_dir)<0.0) N=-N;\n"
    "  float d=dot(q-C,N);\n"
    "  float taper=0.40 + 0.60*sin(3.14159265*clamp(bt,0.0,1.0));\n"
    "  float R=max(u_radius*taper,1.0);\n"
    "  float e=max(u_edgeSoft,0.5);\n"
    // front
    "  vec2 fuv=q/u_pageSize;\n"
    "  vec3 front=texture(u_inputTexture,fuv).rgb;\n"
    "  front*=(1.0-exp(-max(d,0.0)/(R*0.5))*u_shadow);\n"
    // backside band (wrapped, mirrored)
    "  float s=clamp((-d)/R,0.0,1.0); float theta=s*3.14159265; float arc=theta*R;\n"
    "  vec2 pagePt=C - N*arc;\n"
    "  vec2 buv=vec2(1.0-pagePt.x/u_pageSize.x, pagePt.y/u_pageSize.y);\n"
    "  vec3 back=texture(u_inputTexture,buv).rgb;\n"
    "  float hi=exp(-pow((s-0.55)/0.20,2.0))*u_highlight;\n"
    "  float dark=u_backsideDarken*(0.30+0.70*s);\n"
    "  vec3 backCol=back*(1.0-dark)+vec3(hi);\n"
    // revealed
    "  vec3 reveal=texture(u_back,suv).rgb;\n"
    "  reveal*=(1.0-exp(-max((-d)-R,0.0)/(R*0.8))*u_shadow*0.8);\n"
    "  float frontW=smoothstep(-e,e,d);\n"
    "  float revealW=1.0-smoothstep(-R-e,-R+e,d);\n"
    "  float backW=clamp(1.0-frontW-revealW,0.0,1.0);\n"
    "  F=vec4(front*frontW + backCol*backW + reveal*revealW,1.0);\n"
    "}\n";

}

bool PageCurl::Init() {
    return ShaderManager::Register("page_curl", CURL_VS, CURL_FS) != nullptr;
}

void PageCurl::Clean() { /* shader 由 ShaderManager::Clean 统一释放 */ }

bool PageCurl::Ready() { return ShaderManager::Get("page_curl") != nullptr; }

void PageCurl::Render(const Texture* pageTexture, const Texture* backTexture,
                      int width, int height, const PageCurlParams& params) {
    Shader* shader = ShaderManager::Get("page_curl");
    if (!shader || !shader->Valid() || !pageTexture || width <= 0 || height <= 0) return;

    const float W = (float)width, H = (float)height;
    const float ox = params.originX * W, oy = params.originY * H;
    float dx = (1.0f - 2.0f * params.originX);
    float dy = (1.0f - 2.0f * params.originY);
    float dl = std::sqrt(dx * dx + dy * dy);
    if (dl < 1e-4f) { dx = -1.0f; dy = -1.0f; dl = std::sqrt(2.0f); }
    dx /= dl; dy /= dl;

    const float diag = std::sqrt(W * W + H * H);
    float bx = ox + dx * (params.progress * diag);
    float by = oy + dy * (params.progress * diag);
    const float px = -dy, py = dx;        // perpendicular
    const float span = W + H;
    float p0x = bx - px * span, p0y = by - py * span;
    float p3x = bx + px * span, p3y = by + py * span;

    float p1x, p1y, p2x, p2y;
    if (params.explicitControl) {
        p1x = params.control1X * W; p1y = params.control1Y * H;
        p2x = params.control2X * W; p2y = params.control2Y * H;
    } else {
        const float bulge = params.curvature * 0.25f * std::min(W, H);
        p1x = p0x + dx * bulge; p1y = p0y + dy * bulge;
        p2x = p3x + dx * bulge; p2y = p3y + dy * bulge;
    }

    const float radius = std::max(1.0f, params.curlRadius * std::min(W, H));

    shader->Use();
    shader->SetVec2("u_pageSize", W, H);
    shader->SetVec2("uP0", p0x, p0y);
    shader->SetVec2("uP1", p1x, p1y);
    shader->SetVec2("uP2", p2x, p2y);
    shader->SetVec2("uP3", p3x, p3y);
    shader->SetVec2("u_dir", dx, dy);
    shader->SetFloat("u_radius", radius);
    shader->SetFloat("u_curvature", params.curvature);
    shader->SetFloat("u_shadow", params.shadowStrength);
    shader->SetFloat("u_highlight", params.highlightStrength);
    shader->SetFloat("u_backsideDarken", params.backsideDarken);
    shader->SetFloat("u_edgeSoft", params.edgeSoftness);
    shader->SetFloat("u_progress", params.progress);

    // back texture -> unit 1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, backTexture && backTexture->Valid() ? backTexture->id_ : 0);
    shader->SetInt("u_back", 1);
    glActiveTexture(GL_TEXTURE0);

    Renderer::DrawFullscreen(*shader, pageTexture);
    Renderer::CheckError("page curl");
}
