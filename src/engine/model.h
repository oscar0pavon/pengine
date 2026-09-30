#ifndef PE_MODEL_H
#define PE_MODEL_H



#include <engine/images.h>
#include "array.h"

#include <cglm/vec3.h>

#include "renderer/shaders.h"
#include "renderer/vulkan.h"


#include <vulkan/vulkan_core.h>
#include "renderer/vk_buffer.h"

#include "renderer/material.h"

//forward declared, not included: skeletal.h includes this header itself,
//and pe_vk_model_instance_skinned() below only ever needs a pointer. file
//scope matters here, not just "declared somewhere" - a struct tag named
//only inside a parameter list has that declaration's scope, not the whole
//file's, so it would be a different, incompatible PSkin from skeletal.h's
//own full definition rather than the same forward-declared one completed
struct PSkin;

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
descriptor sets so it can carry its own transform. Descriptor sets are
built against the plain (uniform buffer only) layout, matching gui.c's flat-
colour button quads, which is what source must have been drawn with too*/
PModel *pe_vk_model_instance(PModel* model, PModel *source);

/*Same as pe_vk_model_instance(), but for a source drawn with a texture
(pe_vk_descriptor_set_layout_with_texture): the copy's descriptor set is
built against that layout instead and points at source's own texture*/
PModel *pe_vk_model_instance_textured(PModel* model, PModel *source);

/*Same as pe_vk_model_instance_textured(), but for a source loaded with
pe_vk_load_skin() (pe_vk_descriptor_set_layout_skinned): the copy's
descriptor set also binds skin's joint-matrix storage buffer at binding 2.
skin is shared, not copied - see the .c file's comment for what that means
for per-instance animation*/
PModel *pe_vk_model_instance_skinned(PModel *model, PModel *source,
                                     struct PSkin *skin);

int pe_load_model_path(PModel* model, const char *path);

#endif // !PE_MODEL_H
