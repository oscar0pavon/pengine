#version 450
#extension GL_GOOGLE_include_directive : require

#include "terrain_frame.glsl"

layout(location = 0) in vec3 direction;

layout(location = 0) out vec4 out_color;

void main() {
  float height = clamp(normalize(direction).z, 0.0, 1.0);

  //the ground fades into the fog colour, so that is the colour the sky has to
  //meet it with; from there it climbs to the zenith
  float toward_zenith = smoothstep(0.0, 0.6, height);

  out_color = vec4(mix(frame.fog_color.rgb, frame.sky_zenith.rgb, toward_zenith), 1.0);
}
