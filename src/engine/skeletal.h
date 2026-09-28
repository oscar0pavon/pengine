#ifndef PE_SKELETAL_H
#define PE_SKELETAL_H


#include <cglm/cglm.h>

#include "array.h"
#include "model.h"

#include <engine/animation/node.h>

struct Node;

typedef struct Skeletal{
    Node* joints;
    unsigned short int joints_count;
}Skeletal;

//INFO 256, not the 35 this used to be: a classic WoW model's own MAX_BONES is
//0x100, and a player character (all the attachment points a piece of gear
//can ride on, on top of the real skeleton) uses most of it - taurenmale.m2
//alone has 134
#define PE_SKELETAL_JOINTS_MAX 256

typedef struct SkeletalNodeUniform{
    int joint_count;
    mat4 joints_matrix[PE_SKELETAL_JOINTS_MAX];
}SkeletalNodeUniform;

//INFO one skinned mesh: the geometry, its joint hierarchy and the joint
//matrices the vertex shader reads. it used to be a component hung off an
//Element; an application owns it directly now
typedef struct PSkin {
  Array meshes;
  Array distances;
  Array textures;
  PModel *mesh;
  Array joints;
  vec3 bounding_box[2];
  Array animations;
  Array inverse_bind_matrices;
  SkeletalNodeUniform node_uniform;
  Array shader_storage_buffers;
  Array shader_storage_buffers_memory;
} PSkin;

void free_node(Node*);

void get_local_matrix(Node* node, mat4 out_mat);
void get_global_matrix(Node* node, mat4 out_mat);

//loads a rigged model's mesh into model (same as pe_vk_load_model()) and, if
//the glTF file has a skin, its joints/inverse bind matrices/animations into
//skin, which the caller owns (skin->mesh is set to model). a model with no
//skin still loads as a plain mesh; skin is left zeroed
PModel *pe_vk_load_skin(PSkin *skin, PModel *model, const char *path);

//one joint matrix storage buffer per swap chain image - call once after
//pe_vk_load_skin(), before the skin's descriptor sets are written
void pe_vk_skin_create_storage_buffers(PSkin *skin);

//uploads this frame's skin->node_uniform (pe_anim_nodes_update() fills it)
//into the storage buffer the given swap chain image's descriptor set reads
void pe_vk_skin_send_storage_buffer(PSkin *skin, unsigned int image_index);


#endif
