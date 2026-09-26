#version 450
#extension GL_GOOGLE_include_directive : require

#include "terrain_frame.glsl"

layout(location = 0) out vec3 direction;

void main() {
  //one triangle bigger than the screen, from the vertex number alone
  vec2 corner = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
  vec2 screen = corner * 2.0 - 1.0;

  gl_Position = vec4(screen, 1.0, 1.0);

  vec4 world = frame.inverse_view_projection * vec4(screen, 1.0, 1.0);
  direction = world.xyz / world.w - frame.camera_position.xyz;
}
