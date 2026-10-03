#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 light_position;
    vec4 color;
} ubo;

layout(location = 0) out vec3 direction;
layout(location = 1) out float exposure;

void main() {
    //one triangle bigger than the screen, from the vertex number alone
    vec2 corner = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    vec2 screen = corner * 2.0 - 1.0;

    gl_Position = vec4(screen, 1.0, 1.0);

    vec4 world = inverse(ubo.proj * ubo.view) * vec4(screen, 1.0, 1.0);
    direction = world.xyz / world.w - inverse(ubo.view)[3].xyz;
    exposure = ubo.color.x;
}
