#version 450

layout(set = 0, binding = 1) uniform sampler2D base_color_texture;
layout(set = 0, binding = 2) uniform sampler2D metallic_roughness_texture;
layout(set = 0, binding = 3) uniform sampler2D normal_texture;

layout(set = 1, binding = 0) uniform Environment {
    vec4 irradiance[9];
    vec4 sun_direction;
    vec4 sun_color;
    vec4 settings;
} environment;
layout(set = 1, binding = 1) uniform sampler2D panorama;

layout(push_constant) uniform Material {
    vec4 factors;
    vec4 emissive;
} material;

layout(location = 0) in vec3 world_position;
layout(location = 1) in vec3 world_normal;
layout(location = 2) in vec3 vertex_color;
layout(location = 3) in vec2 in_uv;
layout(location = 4) in vec3 camera_position;

layout(location = 0) out vec4 out_color;

const float PI = 3.14159265;

vec2 panorama_uv(vec3 direction) {
    return vec2(atan(direction.x, direction.y) / (2.0 * PI) + 0.5,
                0.5 - asin(clamp(direction.z, -1.0, 1.0)) / PI);
}

//the light a surface facing normal gets from the whole panorama, already
//divided by pi
vec3 irradiance(vec3 n) {
    return max(
        environment.irradiance[0].rgb * 0.282095
      + environment.irradiance[1].rgb * 0.488603 * n.y
      + environment.irradiance[2].rgb * 0.488603 * n.z
      + environment.irradiance[3].rgb * 0.488603 * n.x
      + environment.irradiance[4].rgb * 1.092548 * n.x * n.y
      + environment.irradiance[5].rgb * 1.092548 * n.y * n.z
      + environment.irradiance[6].rgb * 0.315392 * (3.0 * n.z * n.z - 1.0)
      + environment.irradiance[7].rgb * 1.092548 * n.x * n.z
      + environment.irradiance[8].rgb * 0.546274 * (n.x * n.x - n.y * n.y),
        vec3(0.0));
}

//the normal of the surface bent by its normal map, with the frame of the
//texture worked out from how the position and the uv change across the screen,
//so the model needs no tangents
vec3 bent_normal(vec3 n, vec3 position, vec2 uv) {
    vec3 tangent_space = texture(normal_texture, uv).xyz * 2.0 - 1.0;
    tangent_space.xy *= material.factors.z;

    vec3 dp1 = dFdx(position);
    vec3 dp2 = dFdy(position);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);

    vec3 dp2perp = cross(dp2, n);
    vec3 dp1perp = cross(n, dp1);
    vec3 tangent = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 bitangent = dp2perp * duv1.y + dp1perp * duv2.y;

    float scale = inversesqrt(max(dot(tangent, tangent), dot(bitangent, bitangent)));
    return normalize(mat3(tangent * scale, bitangent * scale, n) * tangent_space);
}

float distribution_ggx(float n_dot_h, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float d = n_dot_h * n_dot_h * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

float visibility_smith(float n_dot_v, float n_dot_l, float roughness) {
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float v = n_dot_v * (1.0 - k) + k;
    float l = n_dot_l * (1.0 - k) + k;
    return 1.0 / (4.0 * v * l);
}

//Karis' fit of what the split sum integral of the specular light is: a scale
//and a bias for F0, which saves a lookup texture
vec3 environment_brdf(vec3 f0, float roughness, float n_dot_v) {
    const vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
    const vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);
    vec4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * n_dot_v)) * r.x + r.y;
    vec2 ab = vec2(-1.04, 1.04) * a004 + r.zw;
    return f0 * ab.x + ab.y;
}

//Narkowicz's fit of the ACES filmic curve
vec3 tonemap(vec3 light) {
    return clamp((light * (2.51 * light + 0.03)) / (light * (2.43 * light + 0.59) + 0.14), 0.0, 1.0);
}

void main() {
    vec3 base_color = texture(base_color_texture, in_uv).rgb * vertex_color;
    vec3 metal_rough = texture(metallic_roughness_texture, in_uv).rgb;
    float roughness = clamp(metal_rough.g * material.factors.y, 0.045, 1.0);
    float metallic = clamp(metal_rough.b * material.factors.x, 0.0, 1.0);

    vec3 n = bent_normal(normalize(world_normal), world_position, in_uv);
    vec3 v = normalize(camera_position - world_position);
    float n_dot_v = max(dot(n, v), 1e-4);

    vec3 f0 = mix(vec3(0.04), base_color, metallic);
    vec3 diffuse_color = base_color * (1.0 - metallic);

    //the sun
    vec3 l = normalize(environment.sun_direction.xyz);
    vec3 h = normalize(v + l);
    float n_dot_l = max(dot(n, l), 0.0);
    float n_dot_h = max(dot(n, h), 0.0);
    float v_dot_h = max(dot(v, h), 0.0);
    vec3 fresnel = f0 + (1.0 - f0) * pow(1.0 - v_dot_h, 5.0);
    vec3 specular = distribution_ggx(n_dot_h, roughness) *
                    visibility_smith(n_dot_v, n_dot_l, roughness) * fresnel;
    vec3 direct = (diffuse_color * (1.0 - fresnel) / PI + specular) *
                  environment.sun_color.rgb * n_dot_l;

    //the panorama: lights the surface from every side, and is what a shiny one
    //shows. the rougher it is the blurrier a level of the panorama it reads
    vec3 diffuse_light = irradiance(n) * diffuse_color;
    vec3 reflected = reflect(-v, n);
    vec3 reflection = textureLod(panorama, panorama_uv(reflected),
                                 roughness * environment.settings.y).rgb;
    vec3 specular_light = reflection * environment_brdf(f0, roughness, n_dot_v);

    vec3 color = direct + diffuse_light + specular_light + material.emissive.rgb;
    out_color = vec4(tonemap(color * environment.settings.x), 1.0);
}
