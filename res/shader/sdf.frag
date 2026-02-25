#version 300 es

precision mediump float;
precision mediump sampler2D;

in vec2 v_uv;
in vec4 v_tint;
in float v_size;

out vec4 g_color;

uniform sampler2D u_sampler;

void main()
{
    vec4 texel_color = texture(u_sampler, v_uv);
    // Distance from edge (0 at edge)
    float dist = (1.0 - v_size) - texel_color.r;
    
    // Screen-space derivatives
    float dx = dFdx(dist);
    float dy = dFdy(dist);

    // Convert to pixel distance
    float pixelDist = dist / max(length(vec2(dx, dy)), 1e-5);

    // Anti-aliased alpha (1-pixel smooth edge)
    float alpha = clamp(0.5 - pixelDist, 0.0, 1.0);

    g_color = vec4(v_tint.rgb, v_tint.a * alpha);
}