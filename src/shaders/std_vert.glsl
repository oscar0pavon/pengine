#version 100
attribute vec3 vPosition;
attribute vec2 vUV;
attribute vec3 vColor;
attribute vec3 vNormal;

uniform mat4 MVP; 
uniform mat4 uModel;

varying vec3 FragPosition;
varying vec2 v_TexCoord;
varying vec3 Normal;

void main()
{
//    FragPosition = vec3(uModel * vec4(vPosition,1));
    gl_Position = MVP * vec4(vPosition,1);
    v_TexCoord = vUV;
    Normal = vNormal;
}
//end
