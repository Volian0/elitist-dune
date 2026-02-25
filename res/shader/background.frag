#version 300 es

precision highp float;

in vec2 v_uv;

out vec4 g_color;
 
uniform float u_aspect_ratio;
uniform float u_time;
uniform float u_id;

vec2 hash2(vec2 p)
{
    p = fract(p * vec2(5.3983, 5.4427));
    p += dot(p, p + 3.5453123);
    return fract(vec2(p.x * p.y, p.x + p.y));
}

float voronoi2d(vec2 point) {
  vec2 p = floor(point);
  vec2 f = fract(point);
  float res = 0.0;
  for (int j = -1; j <= 1; j++) {
    for (int i = -1; i <= 1; i++) {
      vec2 b = vec2(i, j);
      vec2 r = vec2(b) - f + hash2(p + b);
      res += 1. / pow(dot(r, r), 8.);
    }
  }
  return pow(1. / res, 0.0625);
}

void main()
{
    vec2 n = vec2(voronoi2d(v_uv * 4.0 * vec2(u_aspect_ratio, 1.0) + u_time)); 
    n.r = 1.0 - n.r;
    
    vec3 col1 = mix(vec3(1.0 - n.r, 1.0, 1.0), vec3(1.0 - n.g, 1.0, 1.0), 1.0 - v_uv.y);
    vec3 col2 = mix(vec3(1.0 - n.r, (n.r + n.g) * 0.5, 1.0), vec3(1.0 - n.g, 1.0, 1.0), 1.0 - v_uv.y);
    vec3 col3 = mix(vec3(1.0 - n.g, 1.0, 1.0), vec3(1.0, max(n.r, 1.0 - n.g), 1.0), 1.0 - v_uv.y);

    vec3 col = mix(mix(col1, col2, min(u_id, 1.0)), col3, max(u_id - 1.0, 0.0));

    // Output to screen
    g_color = vec4(col, 1.0);
}