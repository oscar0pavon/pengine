#ifndef MODEL_H
#define MODEL_H



#include <engine/images.h>
#include "array.h"

#include <cglm/vec3.h>

#include "renderer/shaders.h"
#include "renderer/vulkan.h"


#include <vulkan/vulkan_core.h>
#include "renderer/vk_buffer.h"

#include <GL/gl.h>

#include "renderer/material.h"

typedef struct PMesh{
  Array vertex_array;
  Array index_array;

  VkBuffer vertex_buffer;
  VkBuffer index_buffer;
    
  GLuint vertex_buffer_id;
  GLuint index_buffer_id;
}PMesh;

typedef struct PModel{
    int id;
    unsigned short int texture_count;
    
    Array vertex_array;
    Array index_array;
   
    vec3 min;
    vec3 max;
    

    mat4 model_mat;

    PTexture texture;
    // PTexture textures[4];

    PMaterial material;


    GLuint vertex_buffer_id;
    GLuint index_buffer_id;

    PBuffer vertex_buffer;
    PBuffer index_buffer;

    Array uniform_buffers;
    Array uniform_buffers_memory;
    Array descriptor_sets;
    VkDescriptorPool descriptor_pool;
 
    vec3 position;
    PMesh mesh;
	  bool gpu_ready;

    PShader shader;
    
    GLuint shader_id;

    PUniformBufferObject uniform_buffer_object;
}PModel;

typedef struct DrawData{
    u32 shader;
    u32 texture;
    u32 vertex;
    u32 index;
}DrawData;

static int pe_data_loader_models_loaded_count;

void pe_clean_model(PModel* model);

PModel *pe_vk_load_model(PModel* model, const char *path);

int pe_load_model_path(PModel* model, const char *path);

#endif // !MODEL_H
