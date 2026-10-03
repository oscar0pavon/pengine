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

    //the ray of this corner in the camera's own axes, from the projection's two
    //scales, then turned into the world's by the view's rotation alone. it needs
    //no inverse, and nothing of where the camera is, which a direction does not
    //depend on
    vec3 toward = vec3(screen.x / ubo.proj[0][0], screen.y / ubo.proj[1][1], 1.0);
    direction = transpose(mat3(ubo.view)) * toward;
    exposure = ubo.color.x;
}
