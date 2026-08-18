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

void free_node(Node*);

void get_local_matrix(Node* node, mat4 out_mat);
void get_global_matrix(Node* node, mat4 out_mat);


#endif
