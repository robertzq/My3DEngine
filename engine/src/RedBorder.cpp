#include "Engine/RedBorder.h"
#include "Engine/Log.h"
#include "Engine/PostProcess.h"
#include "Engine/Shader.h"
#include "Engine/ShaderManager.h"

namespace {

const char* RB_VS =
    "#version 330 core\n"
    "layout(location=0) in vec2 aUnit;\n"
    "out vec2 vUv;\n"
    "void main(){ vUv=aUnit; gl_Position=vec4(aUnit*2.0-1.0,0.0,1.0); }\n";

// 边框权重集中在边缘；noise 扭曲边界；高频 jitter + 低频 breathing 分离；轻量色差。
const char* RB_FS =
    "#version 330 core\n"
    "in vec2 vUv; out vec4 F;\n"
    "uniform sampler2D u_inputTexture;\n"
    "uniform vec2 u_resolution;\n"
    "uniform float u_time;\n"
    "uniform float u_intensity, u_borderWidth, u_shakeAmount, u_noiseSpeed, u_redStrength, u_chromatic;\n"
    "float hash(vec2 p){ return fract(sin(dot(p, vec2(127.1,311.7)))*43758.5453123); }\n"
    "float noise(vec2 p){ vec2 i=floor(p), f=fract(p); f=f*f*(3.0-2.0*f);\n"
    "  float a=hash(i), b=hash(i+vec2(1,0)), c=hash(i+vec2(0,1)), d=hash(i+vec2(1,1));\n"
    "  return mix(mix(a,b,f.x), mix(c,d,f.x), f.y); }\n"
    "void main(){\n"
    "  if(u_intensity<=0.0){ F=texture(u_inputTexture, vUv); return; }\n"
    "  vec2 uv=vUv;\n"
    "  float aspect=u_resolution.x/max(u_resolution.y,1.0);\n"
    "  vec2 e=min(uv, 1.0-uv);\n"
    "  float edgeDist=min(e.x, e.y);\n"
    "  float bw=max(u_borderWidth, 0.001);\n"
    "  // 低频 breathing\n"
    "  float breath=0.85+0.15*sin(u_time*2.2);\n"
    "  // 不规则边界（noise 扭曲），中低频为主避免雪花感\n"
    "  float n = noise(vec2(uv.x*aspect, uv.y)*4.0 + vec2(u_time*u_noiseSpeed, u_time*u_noiseSpeed*1.3));\n"
    "  float boundary = bw*(0.55 + 0.45*n);\n"
    "  float w = (1.0 - smoothstep(boundary*0.5, boundary, edgeDist)) * u_intensity * breath;\n"
    "  w = clamp(w, 0.0, 1.0);\n"
    "  // 高频小抖动（只作用于边缘权重区域）\n"
    "  float tt=u_time*u_noiseSpeed;\n"
    "  vec2 jit=vec2(noise(vec2(tt, uv.y*40.0))-0.5, noise(vec2(uv.x*40.0, tt+7.0))-0.5);\n"
    "  vec2 suv=clamp(uv + jit*u_shakeAmount*w, 0.0, 1.0);\n"
    "  // 轻量 chromatic distortion\n"
    "  float ca=u_chromatic*w;\n"
    "  float r=texture(u_inputTexture, clamp(suv+vec2(ca,0.0),0.0,1.0)).r;\n"
    "  float g=texture(u_inputTexture, suv).g;\n"
    "  float b=texture(u_inputTexture, clamp(suv-vec2(ca,0.0),0.0,1.0)).b;\n"
    "  vec3 col=vec3(r,g,b);\n"
    "  // 红色叠加（边缘为主）\n"
    "  vec3 red=vec3(1.0,0.12,0.12);\n"
    "  col=mix(col, red, clamp(w*u_redStrength,0.0,0.9));\n"
    "  col+=vec3(w*u_redStrength*0.18, 0.0, 0.0);\n"
    "  F=vec4(col,1.0);\n"
    "}\n";

bool active = false;
float elapsed = 0.0f;
float duration = 0.0f;
RedBorderParams params;

} // namespace

bool RedBorder::Init() {
    if (!ShaderManager::Register("red_border", RB_VS, RB_FS)) return false;

    PostEffect fx;
    fx.name = "red_border";
    fx.shader = ShaderManager::Get("red_border");
    fx.enabled = false;             // 空闲时不参与 pass
    fx.SetFloat("u_intensity", 0.0f);
    fx.SetFloat("u_borderWidth", params.borderWidth);
    fx.SetFloat("u_shakeAmount", params.shakeAmount);
    fx.SetFloat("u_noiseSpeed", params.noiseSpeed);
    fx.SetFloat("u_redStrength", params.redStrength);
    fx.SetFloat("u_chromatic", params.chromatic);
    PostProcess::AddFinalEffect(fx);
    return true;
}

void RedBorder::Clean() {}

void RedBorder::Trigger(float dur, const RedBorderParams& p) {
    active = true;
    elapsed = 0.0f;
    duration = dur > 0.0f ? dur : 0.001f;
    params = p;

    if (PostEffect* fx = PostProcess::FinalEffect("red_border")) {
        fx->enabled = true;
        fx->SetFloat("u_borderWidth", params.borderWidth);
        fx->SetFloat("u_shakeAmount", params.shakeAmount);
        fx->SetFloat("u_noiseSpeed", params.noiseSpeed);
        fx->SetFloat("u_redStrength", params.redStrength);
        fx->SetFloat("u_chromatic", params.chromatic);
    }
    LOG_INFO("RedBorder: 触发 " << duration << "s");
}

void RedBorder::Update(float dt) {
    if (!active) return;
    elapsed += dt;

    const float fadeIn = 0.25f;
    const float fadeOut = 0.6f;
    float intensity;
    if (elapsed <= fadeIn) intensity = elapsed / fadeIn;
    else if (elapsed >= duration - fadeOut) intensity = (duration - elapsed) / fadeOut;
    else intensity = 1.0f;
    intensity = intensity < 0.0f ? 0.0f : (intensity > 1.0f ? 1.0f : intensity);

    if (PostEffect* fx = PostProcess::FinalEffect("red_border")) {
        fx->SetFloat("u_intensity", intensity);
    }

    if (elapsed >= duration) {
        active = false;
        if (PostEffect* fx = PostProcess::FinalEffect("red_border")) fx->enabled = false;
    }
}

bool RedBorder::Active() { return active; }
