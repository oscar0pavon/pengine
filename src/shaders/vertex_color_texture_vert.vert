#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 light_position;
} ubo;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 out_uv;

const float AMBIENT = 0.35;

void main() {
    vec4 position_world = ubo.model * vec4(position, 1.0);
    gl_Position = ubo.proj * ubo.view * position_world;

    vec3 normal_world = normalize(mat3(ubo.model) * normal);
    vec3 direction_to_light = normalize(ubo.light_position.xyz - position_world.xyz);

    float light = AMBIENT + (1.0 - AMBIENT) * max(dot(normal_world, direction_to_light), 0.0);
    frag_color = color * light;
    out_uv = uv;
}
