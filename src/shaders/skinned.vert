#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 light_position;
} ubo;

layout(std430, set = 0, binding = 2) readonly buffer JointMatrices {
	mat4 joint_matrices[];
};

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;
layout(location = 4) in vec4 joint;
layout(location = 5) in vec4 weight;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 out_uv;

const vec3 LIGHT_DIRECTION = normalize(vec3(3.0,3.0,3.0));

const float AMBIENT = 0.45f;

void main() {

mat4 skin_mat= 
		weight.x * joint_matrices[int(joint.x)] +
		weight.y * joint_matrices[int(joint.y)] +
		weight.z * joint_matrices[int(joint.z)] +
		weight.w * joint_matrices[int(joint.w)];

    vec4 position_world = ubo.model * vec4(position, 1.0) ;

    gl_Position = ubo.proj * ubo.view * ubo.model * skin_mat* vec4(position,1.0);
   
    vec3 normal_world_space = normalize(mat3(ubo.model * skin_mat) * normal);

    vec3 direction_to_light = ubo.light_position.xyz - position_world.xyz;

    float attenuation = 1.0 / dot(direction_to_light, direction_to_light);

    vec3 light_color = vec3(1,1,1) * 1;

    vec3 ambient_light = vec3(1,1,1) * AMBIENT;

    vec3 diffuse_light = light_color * max(dot(normal_world_space, normalize(direction_to_light)),0) ;

    frag_color = (diffuse_light + ambient_light) * vec3(1,1,1);

    out_uv = uv;

}
