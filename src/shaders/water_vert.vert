#version 450
#extension GL_GOOGLE_include_directive : require

#include "terrain_frame.glsl"

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 params;

layout(location = 0) out vec3 out_position;
layout(location = 1) out vec2 out_params;

void main() {
  out_position = position;
  out_params = params;

  gl_Position = frame.projection * frame.view * vec4(position, 1.0);
}
