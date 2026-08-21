#include "node.h"
#include <engine/skeletal.h>
#include <engine/animation/animation.h>


void pe_node_translate(Node* node, vec3 position){
  glm_vec3_add(node->translation,position, node->translation) ;
//  glm_vec3_copy(position,node->translation);

    

}
void pe_node_rotate(Node* node, float angle, vec3 axis){


    versor new_rot_quat;
    glm_quat_identity(new_rot_quat) ;
    
    glm_quatv(new_rot_quat, glm_rad(angle), axis);

    glm_quat_mul(node->rotation, new_rot_quat, node->rotation);

    

}

Node* pe_node_by_name(Array* array, const char* name){
  for( int i = 0; i < array->count ; i++ ){
    Node* node = array_get(array,i);
    if( strcmp( node->name , name ) == 0){
      return node;
    }
  }
}



