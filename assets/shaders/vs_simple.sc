$input a_position
#include <bgfx_shader.sh>

uniform mat4 u_mvp;

void main()
{
    gl_Position = mul(u_mvp, vec4(a_position, 1.0));
}
