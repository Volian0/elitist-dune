#version 300 es

layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec4 a_tint;
layout (location = 3) in float a_size;

out vec2 v_uv;
out vec4 v_tint;
out float v_size;

void main()
{
    v_uv = a_uv;
    v_tint = a_tint;
    v_size = a_size;
    gl_Position = vec4(a_pos, 0.0, 1.0);
}