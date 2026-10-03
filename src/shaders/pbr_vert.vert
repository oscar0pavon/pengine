#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 light_position;
    vec4 color;
} ubo;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 world_position;
layout(location = 1) out vec3 world_normal;
layout(location = 2) out vec3 vertex_color;
layout(location = 3) out vec2 out_uv;
layout(location = 4) out vec3 camera_position;

void main() {
    vec4 world = ubo.model * vec4(position, 1.0);
    gl_Position = ubo.proj * ubo.view * world;

    world_position = world.xyz;
    world_normal = mat3(ubo.model) * normal;
    vertex_color = color;
    out_uv = uv;

    //the view is a rotation and a translation, so the eye is where it undoes
    camera_position = -transpose(mat3(ubo.view)) * ubo.view[3].xyz;
}
