#ifndef PAVON_ENGINE_SKELETAL_H
#define PAVON_ENGINE_SKELETAL_H


#include <cglm/cglm.h>

#include "array.h"
#include "model.h"

#include <engine/animation/node.h>

struct Node;

typedef struct Skeletal{
    Node* joints;
    unsigned short int joints_count;
}Skeletal;

typedef struct SkeletalNodeUniform{
    int joint_count;
    mat4 joints_matrix[35];
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


#endif
