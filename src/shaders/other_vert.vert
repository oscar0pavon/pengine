#version 450

//layout(location = 0) in vec3 in_position;

void main() {
    vec2 positions[3] = vec2[3](
    vec2(0.0, -0.3),
    vec2(0.3, 0.3),
    vec2(-0.3, 0.3)
    );
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
 //   gl_Position = vec4(in_position, 1.0);
}
