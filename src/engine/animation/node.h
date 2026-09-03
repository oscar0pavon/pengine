#ifndef PE_ANIMATION_NODE_H

#define PE_ANIMATION_NODE_H

#include <engine/types.h>

typedef struct Node{
    int id;
    char name[50];
    struct Node* parent;
    vec3 translation;
    versor rotation;
}Node;

Node* pe_node_by_name(Array* array, const char* name);
void pe_node_rotate(Node* node, float angle, vec3 axis);
void pe_node_translate(Node* node, vec3 position);

#endif

