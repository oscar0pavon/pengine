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

//one mesh primitive's own record from pe_load_mesh(), kept after load so
//pe_model_set_active_geosets() can rebuild PModel.index_array against an
//arbitrary active set without re-parsing the glTF file. first_index/
//index_count are a slice of PModel.all_indices, not of index_array itself
typedef struct PGeosetBatch {
  bool tagged;     //false: no {"geoset":N} extras at all (pe_primitive_geoset()) -
                   //always drawn, regardless of any active set
  bool is_default; //true if pe_load_mesh() put this primitive's indices in
                   //index_array at load time (pe_primitive_is_default())
  u32 geoset;      //only meaningful when tagged is true
  u32 first_index;
  u32 index_count;
} PGeosetBatch;

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

    //geoset support (pe_model_set_active_geosets()): every primitive's own
    //indices, concatenated in file order regardless of default-ness, and
    //one PGeosetBatch per primitive recording where in here it sits. empty
    //for a model pe_load_mesh() never saw a geoset extra on, or that was
    //never loaded through it at all - see pe_model_set_active_geosets()'s
    //own doc comment before calling it on such a model
    Array all_indices;
    Array geoset_batches;

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
// Geosets
//
// A model carries every alternative each geoset group has (every hairstyle,
// every glove, bare forearms too), but pe_load_mesh() only ever draws one -
// its own guessed default - since loading has no character/equipment data to
// pick a real one from (see pe_primitive_is_default()'s own doc comment in
// model.c). These two let a caller override that choice once it does have
// that data, without needing to reload or re-parse the model.
//

//the geoset ids pe_load_mesh() actually put in model->index_array at load
//time - up to out_max, returns how many there are in total even if that is
//more. a caller building its own active set (a character's equipped/bare
//geosets) starts from this rather than re-deriving pe_primitive_is_default()'s
//own per-group "no bare variant" rule a second time
u32 pe_model_default_geosets(PModel *model, u32 *out, u32 out_max);

//whether this model carries any primitive tagged with this exact geoset id
//at all (default or not) - a caller picking a real equipped variant (not a
//bare default) needs this first, since asking for one the model does not
//have empties index_array of that group instead of falling back to bare
bool pe_model_has_geoset(PModel *model, u32 geoset);

//rebuilds model->index_array to draw every untagged primitive (no
//{"geoset":N} extras at all) plus every primitive whose own geoset id is in
//geosets, and re-uploads the GPU index buffer to match. unlike pe_load_mesh()'s
//own default selection, this takes the caller's set exactly as given: two
//variants of the same group both draw if both are named, and a group named
//by neither draws nothing, not a bare fallback - getting a sane result for
//every group not being overridden is the caller's job (start from
//pe_model_default_geosets() and only replace the groups actually being
//equipped). only meaningful on a model pe_vk_load_model()/pe_vk_load_skin()
//already loaded from a glTF with geoset extras - one that never went
//through pe_load_mesh(), or whose mesh had none, has empty geoset_batches
//and this would empty index_array instead of leaving it alone
void pe_model_set_active_geosets(PModel *model, const u32 *geosets, u32 count);

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
