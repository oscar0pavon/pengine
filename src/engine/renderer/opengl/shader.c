//
// Created by pavon on 6/24/19.
//

#include "shader.h"
#include <engine/file_loader.h>
#include <engine/threads.h>
#include <string.h>
#include <engine/interruptions.h>
#include <engine/files.h>

bool pe_shader_get_error(GLuint shader, GLenum info_type,
                         const char *path_for_error_debug) {

  char *status_description;
  GLint passed = GL_TRUE;
  GLint lenght;
  if (info_type == GL_COMPILE_STATUS) {
    glGetShaderiv(shader, info_type, &passed);
    if (passed == GL_FALSE) {
      status_description = "Compile fail";
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &lenght);
      GLchar error[lenght];
      glGetShaderInfoLog(shader, lenght, &lenght, &error[0]);
      LOG("%s shader: %s\n", status_description, path_for_error_debug);
      LOG("Error Compiling: %s\n", error);
      //debug_break();     
      return true;
    }
  }
  if (info_type == GL_LINK_STATUS) {
    glGetProgramiv(shader, info_type, &passed);
    if (passed == GL_FALSE) {
      status_description = "Link fail";
      glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &lenght);
      GLchar error[lenght];
      glGetProgramInfoLog(shader, lenght, &lenght, error);
      LOG("%s shader: %s\n", status_description, path_for_error_debug);
      LOG("Error Linking: %s\n", error);
      return true;
    }
  }
  return false;
}

GLuint compile_shader(const char* src , GLenum type){
    GLuint shader = glCreateShader(type);
    glShaderSource(shader,1,&src,0);

    glCompileShader(shader);

    char* error_description;
    if(type == GL_FRAGMENT_SHADER){
        error_description = "fragment shader";
    }else if (type == GL_VERTEX_SHADER){
        error_description = "vertex shader";
    }

    if (pe_shader_get_error(shader, GL_COMPILE_STATUS, error_description)) {
        return PE_SHADER_ERROR;
    }

    return shader;
}

GLuint pe_shader_load_src_and_create(const char *path, GLenum shader_type) {
    File new_file;
    load_file(path, &new_file);
    GLuint shader = compile_shader(new_file.data, shader_type);
    if (shader == PE_SHADER_ERROR) {
        LOG("## ERROR Shader %s", path);
        return PE_SHADER_ERROR;
    }
    return shader;
}

bool pe_shader_compile_std() {
#ifdef LINUX

    standart_vertex_shader = pe_shader_load_src_and_create(
        file_std_vert,
        GL_VERTEX_SHADER);
    standart_fragment_shader = pe_shader_load_src_and_create(
        file_diffuse_frag,
        GL_FRAGMENT_SHADER);

    shader_skin_vertex = pe_shader_load_src_and_create(
        file_skin_vertex_shader,
        GL_VERTEX_SHADER);
#endif
#ifdef ANDROID
    // ########## on android systems files are not in the same location
    // #########

    //  standart_vertex_shader = compile_shader(pe_shader_src_std_vert,
    //  GL_VERTEX_SHADER);
    standart_vertex_shader = pe_shader_load_src_and_create(
        file_std_vert, GL_VERTEX_SHADER);
    standart_fragment_shader = pe_shader_load_src_and_create(
        file_diffuse_frag,
        GL_FRAGMENT_SHADER);

    shader_skin_vertex = pe_shader_load_src_and_create(
        file_skin_vertex_shader,
        GL_VERTEX_SHADER);
    shader_skin_diffuse = pe_shader_load_src_and_create(
        file_diffuse_frag,
        GL_FRAGMENT_SHADER);

#endif

    shader_source_color_fragment_shader =
        compile_shader(pe_shader_src_color, GL_FRAGMENT_SHADER);
}

GLuint create_engine_shader(GLuint vertex, GLuint fragment) {
    shader_count++;
    GLuint new_shader = glCreateProgram();
    glAttachShader(new_shader, vertex);
    glAttachShader(new_shader, fragment);
    glLinkProgram(new_shader);
    pe_shader_get_error(new_shader, GL_LINK_STATUS, "Link status");
    glUseProgram(new_shader);
    return new_shader;
}

void pe_shader_new(PEShaderCreation *creation) {
    thread_main.wait = true;
    GLuint shader = create_engine_shader(creation->vertex, creation->pixel);
    memcpy(creation->shader, &shader, sizeof(shader));
    thread_main.wait = false;
}

void pe_shader_create(PEShaderCreation* creation){
    creation->model->shader =
        create_engine_shader(creation->vertex, creation->pixel);
    thread_main.wait = false;
    LOG("shader created\n");
}
