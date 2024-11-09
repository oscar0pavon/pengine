#version 100 
precision mediump float;
uniform vec4 color;
varying vec3 FragPosition;
varying vec3 Normal;
void main()
{
  vec3 norm = normalize(Normal);
//  norm = vec3(1,1,1);
  vec3 light_pos = vec3(3,3,3);
  vec3 ligth_dir = normalize(light_pos - FragPosition);
  float diff = max(dot(norm,ligth_dir),0.0);
  vec3 diffuse = diff * vec3(color.x, color.y, color.z);
  gl_FragColor = vec4(diffuse,1.0);
  
  //gl_FragColor = vec4(color.x,color.y,color.z,1.0);

}
//end$
