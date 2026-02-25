#version 300 es

precision highp float;
precision mediump sampler2D;

in vec2 v_uv;
in vec4 v_tint;

out vec4 g_color;

uniform sampler2D u_sampler;

void main()
{
    vec4 texel_color = texture(u_sampler, v_uv);
    g_color = texel_color * v_tint;
}