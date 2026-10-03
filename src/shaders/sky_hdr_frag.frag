#version 450

layout(location = 0) in vec3 direction;
layout(location = 1) in float exposure;

layout(location = 0) out vec4 out_color;

layout(binding = 1) uniform sampler2D panorama;

const float PI = 3.14159265;

//Narkowicz's fit of the ACES filmic curve
vec3 tonemap(vec3 light) {
    return clamp((light * (2.51 * light + 0.03)) / (light * (2.43 * light + 0.59) + 0.14), 0.0, 1.0);
}

void main() {
    vec3 toward = normalize(direction);

    //the world is Z up, and the panorama's middle is toward +Y
    vec2 uv = vec2(atan(toward.x, toward.y) / (2.0 * PI) + 0.5,
                   0.5 - asin(clamp(toward.z, -1.0, 1.0)) / PI);

    out_color = vec4(tonemap(texture(panorama, uv).rgb * exposure), 1.0);
}
