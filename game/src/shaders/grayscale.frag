#version 330 core
in vec2 vUv;
out vec4 FragColor;
uniform sampler2D u_texture;
uniform vec4 u_tint;
void main() {
    vec4 c = texture(u_texture, vUv);
    float g = dot(c.rgb, vec3(0.299, 0.587, 0.114));
    FragColor = vec4(g, g, g, c.a) * u_tint;
}
