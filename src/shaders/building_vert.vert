#version 450
#extension GL_GOOGLE_include_directive : require

#include "terrain_frame.glsl"

layout(push_constant) uniform Placement {
  mat4 model;
} placement;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec4 color;

layout(location = 0) out vec3 out_position;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec2 out_uv;
layout(location = 3) out vec4 out_color;

void main() {
  vec4 world = placement.model * vec4(position, 1.0);

  out_position = world.xyz;
  //the placement is a rotation, and a reflection of one axis, and no scale, so
  //the normals turn with it as they are
  out_normal = mat3(placement.model) * normal;
  out_uv = uv;
  out_color = color;

  gl_Position = frame.projection * frame.view * world;
}
