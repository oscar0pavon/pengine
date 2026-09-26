#version 450
#extension GL_GOOGLE_include_directive : require

#include "terrain_frame.glsl"

layout(push_constant) uniform Cutoff {
  layout(offset = 64) float alpha;
} cutoff;

layout(set = 1, binding = 0) uniform sampler2D albedo;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec4 color;

layout(location = 0) out vec4 out_color;

void main() {
  vec4 texel = texture(albedo, uv);

  //a cutoff of 0 keeps everything, and a material that is cut out, a window or
  //a leaf, sets it to where the mostly transparent part begins
  if (texel.a < cutoff.alpha)
    discard;

  vec3 to_camera = frame.camera_position.xyz - position;
  float distance_to_camera = length(to_camera);

  //a wall is one sheet with two sides, and the side the camera sees is lit
  vec3 facing = normalize(normal);
  if (dot(facing, to_camera) < 0.0)
    facing = -facing;

  vec3 to_light = normalize(-frame.light_direction.xyz);
  float diffuse = max(dot(facing, to_light), 0.0);

  //the colour is the light painted into the building's rooms by its makers
  vec3 lit = texel.rgb * color.rgb * (frame.ambient_color.rgb + diffuse * frame.light_color.rgb);

  float fog_start = frame.fog_range.x;
  float fog_end = frame.fog_range.y;
  float clear = clamp((fog_end - distance_to_camera) / (fog_end - fog_start), 0.0, 1.0);

  out_color = vec4(mix(frame.fog_color.rgb, lit, clear), 1.0);
}
