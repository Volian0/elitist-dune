#version 300 es

layout (location = 0) in vec4 a_pos;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec4 a_tint;

out vec2 v_uv;
out vec4 v_tint;

void main()
{
    v_uv = a_uv;
    v_tint = a_tint;
    gl_Position = a_pos;
}