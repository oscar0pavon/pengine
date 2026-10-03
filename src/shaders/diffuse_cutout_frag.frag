#version 450

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

layout(binding = 1) uniform sampler2D texture_sampler;

const float ALPHA_CUTOFF = 0.5;

void main() {
    vec4 texel = texture(texture_sampler, in_uv);
    if (texel.a < ALPHA_CUTOFF)
        discard;

    out_color = vec4(in_color * texel.rgb, 1.0);
}
