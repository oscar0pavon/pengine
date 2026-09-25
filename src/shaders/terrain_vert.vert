#version 450
#extension GL_GOOGLE_include_directive : require

#include "terrain_frame.glsl"

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec2 layer_uv;

layout(location = 0) out vec3 out_position;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec2 out_uv;
layout(location = 3) out vec2 out_alpha_uv;

const float CHUNKS_PER_SIDE = 16.0;

void main() {
  //the draw of chunk n is issued with firstInstance = n, and the alpha maps of
  //all 256 chunks sit in one atlas, 16 by 16
  float chunk = float(gl_InstanceIndex);
  vec2 chunk_cell = vec2(mod(chunk, CHUNKS_PER_SIDE), floor(chunk / CHUNKS_PER_SIDE));

  out_position = position;
  out_normal = normal;
  out_uv = uv;
  out_alpha_uv = (chunk_cell + layer_uv) / CHUNKS_PER_SIDE;

  gl_Position = frame.projection * frame.view * vec4(position, 1.0);
}
