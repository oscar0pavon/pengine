#ifndef MODEL_H
#define MODEL_H



#include <engine/images.h>
#include "array.h"

#include <cglm/vec3.h>

#include "renderer/shaders.h"
#include "renderer/vulkan.h"


#include <vulkan/vulkan_core.h>
#include "renderer/vk_buffer.h"

#include "renderer/material.h"

typedef struct PMesh{
  Array vertex_array;
  Array index_array;

  VkBuffer vertex_buffer;
  VkBuffer index_buffer;
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

    PUniformBufferObject uniform_buffer_object;
}PModel;

static int pe_data_loader_models_loaded_count;

void pe_clean_model(PModel* model);

PModel *pe_vk_load_model(PModel* model, const char *path);

//
// Transform
//
// These write PModel.model_mat, which is what an application copies into
// uniform_buffer_object.model before it draws. There is no scene graph left to
// hold a transform for a model, so the model carries its own.
//

/*Set model_mat back to the identity and zero the stored position*/
void pe_model_transform_reset(PModel* model);

/*Compose model_mat as translate * rotate * scale, in that order, replacing
whatever was there. angle is in degrees*/
void pe_model_transform(PModel* model, vec3 position, float angle, vec3 axis,
                        vec3 scale);

/*Move to position, keeping the rotation and scale already in model_mat*/
void pe_model_set_position(PModel* model, vec3 position);

void pe_model_translate(PModel* model, vec3 offset);

/*angle is in degrees*/
void pe_model_rotate(PModel* model, float angle, vec3 axis);

void pe_model_scale(PModel* model, vec3 scale);

/*Share source's geometry, but give model its own uniform buffers and
descriptor sets so it can carry its own transform*/
PModel *pe_vk_model_instance(PModel* model, PModel *source);

int pe_load_model_path(PModel* model, const char *path);

#endif // !MODEL_H
