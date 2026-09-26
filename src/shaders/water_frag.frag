#version 450
#extension GL_GOOGLE_include_directive : require

#include "terrain_frame.glsl"

//x is how deep, 0 at the shore to 1, and y is the kind of liquid
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 params;

layout(location = 0) out vec4 out_color;

const int MAGMA = 2;
const int SLIME = 3;

//waves of three sizes, each travelling its own way, added up as slopes. that
//gives the surface a normal that moves without a texture to scroll
vec3 surface_normal(vec2 point, float time) {
  vec2 slope = vec2(0.0);

  slope += vec2(cos(point.x * 0.55 + time * 1.3),
                cos(point.y * 0.47 - time * 1.1)) * 0.06;
  slope += vec2(cos((point.x + point.y) * 1.3 + time * 2.1),
                cos((point.x - point.y) * 1.1 - time * 1.7)) * 0.03;
  slope += vec2(cos(point.y * 2.9 + time * 3.0),
                cos(point.x * 3.3 - time * 2.6)) * 0.015;

  return normalize(vec3(-slope, 1.0));
}

vec3 sky_color(vec3 direction) {
  float height = clamp(direction.z, 0.0, 1.0);
  return mix(frame.fog_color.rgb, frame.sky_zenith.rgb, smoothstep(0.0, 0.6, height));
}

vec3 with_fog(vec3 color, float distance_to_camera) {
  float fog_start = frame.fog_range.x;
  float fog_end = frame.fog_range.y;
  float clear = clamp((fog_end - distance_to_camera) / (fog_end - fog_start), 0.0, 1.0);

  return mix(frame.fog_color.rgb, color, clear);
}

void main() {
  int liquid = int(params.y + 0.5);
  float depth = params.x;
  float time = frame.time.x;

  vec3 to_camera = frame.camera_position.xyz - position;
  float distance_to_camera = length(to_camera);
  vec3 view = to_camera / distance_to_camera;

  if (liquid == MAGMA) {
    float glow = 0.5 + 0.5 * sin(position.x * 0.4 + time) * sin(position.y * 0.35 - time * 0.7);
    out_color = vec4(with_fog(mix(vec3(0.85, 0.22, 0.02), vec3(1.0, 0.65, 0.12), glow), distance_to_camera), 1.0);
    return;
  }

  vec3 normal = surface_normal(position.xy, time);
  vec3 reflected = reflect(-view, normal);

  float cos_view = max(dot(normal, view), 0.0);
  float fresnel = 0.03 + 0.97 * pow(1.0 - cos_view, 5.0);

  vec3 shallow = liquid == SLIME ? vec3(0.35, 0.55, 0.15) : vec3(0.30, 0.58, 0.58);
  vec3 deep = liquid == SLIME ? vec3(0.10, 0.22, 0.05) : vec3(0.02, 0.15, 0.24);
  vec3 body = mix(shallow, deep, depth) * (0.55 + frame.ambient_color.rgb);

  vec3 to_light = normalize(-frame.light_direction.xyz);
  float glint = pow(max(dot(reflected, to_light), 0.0), 220.0);

  vec3 color = mix(body, sky_color(reflected), fresnel) + glint * frame.light_color.rgb;

  //shallow water lets the bottom show through and deep water does not
  float alpha = clamp(mix(0.25, 0.92, sqrt(depth)) + fresnel * 0.4, 0.0, 1.0);

  out_color = vec4(with_fog(color, distance_to_camera), alpha);
}
