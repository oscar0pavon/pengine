#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 light_position;
    vec4 color;
} ubo;

layout(location = 0) in vec3 position;

layout(location = 0) out vec4 frag_color;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(position, 1.0);

    frag_color = ubo.color;
}
