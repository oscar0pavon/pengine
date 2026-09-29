#include "node.h"

#include <engine/animation/animation.h>
#include <engine/skeletal.h>

void pe_node_translate(Node *node, vec3 position) {
  glm_vec3_add(node->translation, position, node->translation);
}

void pe_node_rotate(Node *node, float angle, vec3 axis) {
  versor rotation;
  glm_quatv(rotation, glm_rad(angle), axis);
  glm_quat_mul(node->rotation, rotation, node->rotation);
}

Node *pe_node_by_name(Array *array, const char *name) {
  for (int i = 0; i < array->count; i++) {
    Node *node = array_get(array, i);
    if (strcmp(node->name, name) == 0)
      return node;
  }
  return NULL;
}
