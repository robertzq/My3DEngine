#version 330 core
in vec2 vUv;
out vec4 FragColor;
uniform sampler2D u_texture;
uniform vec4 u_tint;
uniform float u_intensity;    // 0..1
uniform vec3 u_flashColor;
void main() {
    vec4 c = texture(u_texture, vUv);
    float k = clamp(u_intensity, 0.0, 1.0);
    vec3 rgb = mix(c.rgb, u_flashColor, k);
    FragColor = vec4(rgb, c.a) * u_tint;
}
