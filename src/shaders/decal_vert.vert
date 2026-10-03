#version 450

layout(push_constant) uniform PushConstants {
    mat4 view_projection;
} pc;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_color;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec4 out_color;

void main() {
    gl_Position = pc.view_projection * vec4(in_position, 1.0);
    out_uv = in_uv;
    out_color = in_color;
}
