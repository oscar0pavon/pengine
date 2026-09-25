#version 450
#extension GL_GOOGLE_include_directive : require

#include "terrain_frame.glsl"

layout(set = 1, binding = 0) uniform sampler2D layers[4];
layout(set = 1, binding = 1) uniform sampler2D alpha_atlas;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec2 alpha_uv;

layout(location = 0) out vec4 out_color;

//no branch on the coverage: a texture fetch inside a branch that neighbouring
//pixels do not all take has undefined derivatives, and so undefined mip levels
vec3 blend_layer(vec3 color, sampler2D layer, float coverage) {
  return mix(color, texture(layer, uv).rgb, coverage);
}

void main() {
  vec3 coverage = texture(alpha_atlas, alpha_uv).rgb;

  vec3 color = texture(layers[0], uv).rgb;
  color = blend_layer(color, layers[1], coverage.r);
  color = blend_layer(color, layers[2], coverage.g);
  color = blend_layer(color, layers[3], coverage.b);

  vec3 to_light = normalize(-frame.light_direction.xyz);
  float diffuse = max(dot(normalize(normal), to_light), 0.0);
  vec3 lit = color * (frame.ambient_color.rgb + diffuse * frame.light_color.rgb);

  float fog_start = frame.fog_range.x;
  float fog_end = frame.fog_range.y;
  float distance_to_camera = length(frame.camera_position.xyz - position);
  float clear = clamp((fog_end - distance_to_camera) / (fog_end - fog_start), 0.0, 1.0);

  out_color = vec4(mix(frame.fog_color.rgb, lit, clear), 1.0);
}
