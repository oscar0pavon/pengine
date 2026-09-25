layout(set = 0, binding = 0) uniform TerrainFrame {
  mat4 view;
  mat4 projection;
  vec4 light_direction;
  vec4 light_color;
  vec4 ambient_color;
  vec4 camera_position;
  vec4 fog_color;
  vec4 fog_range;
} frame;
