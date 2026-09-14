#version 330 core
layout(location = 0) in vec2 aUnit;
uniform vec4 uDstRect;      // x,y,w,h (pixels)
uniform vec4 uUvRect;       // u,v,uw,uh (normalized)
uniform vec2 u_resolution;
uniform vec2 uFlip;
out vec2 vUv;
void main() {
    vec2 uv = uUvRect.xy + aUnit * uUvRect.zw;
    if (uFlip.x > 0.5) uv.x = uUvRect.x + uUvRect.z - (uv.x - uUvRect.x);
    if (uFlip.y > 0.5) uv.y = uUvRect.y + uUvRect.w - (uv.y - uUvRect.y);
    vUv = uv;
    vec2 px = uDstRect.xy + aUnit * uDstRect.zw;
    float ndcX = (px.x / u_resolution.x) * 2.0 - 1.0;
    float ndcY = 1.0 - (px.y / u_resolution.y) * 2.0;
    gl_Position = vec4(ndcX, ndcY, 0.0, 1.0);
}
