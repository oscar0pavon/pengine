#version 450
#extension GL_GOOGLE_include_directive : require

#include "terrain_frame.glsl"

//blend is 0 for a solid material, 1 for one blended over what is behind it and
//2 for one added to it
layout(push_constant) uniform Material {
  layout(offset = 64) float cutoff;
  layout(offset = 68) float blend;
} material;

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
  if (texel.a < material.cutoff)
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

  //what is added to the picture fades to nothing in the distance, and not to
  //the fog's colour, which added on would brighten what is behind it
  vec3 fog = material.blend == 2.0 ? vec3(0.0) : frame.fog_color.rgb;
  float alpha = material.blend == 0.0 ? 1.0 : texel.a;

  out_color = vec4(mix(fog, lit, clear), alpha);
}
