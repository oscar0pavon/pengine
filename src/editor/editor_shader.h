/*Create 2019-09-09 by pavon */
#ifndef EDITOR_SHADER_H
#define EDITOR_SHADER_H

static const char*  skeletal_blue_joint_source = "#version 100 \n\
precision mediump float; \
    void main()\
    {\
        gl_FragColor = vec4(0,0,1,1);\
    }\
//end";



GLuint skeletal_blue_shader;

#endif // !EDITOR_SHADER_H
