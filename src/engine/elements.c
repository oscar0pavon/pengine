#include "elements.h"
#include "engine.h"

void pe_element_set_scale(vec3 translation){
    TransformComponent* transform = pe_comp_get(TRASNFORM_COMPONENT);
    if(!transform)
        return;

    glm_vec3_add(transform->scale,translation,transform->scale);
    vec3 identity;
    glm_vec3_copy(GLM_VEC3_ONE,identity);
    glm_vec3_add(identity,translation,translation);

    glm_scale(transform->model_matrix,translation);
    
    for(int i = 0; i<selected_element->components.count; i++){
        ComponentDefinition* component = array_get(&selected_element->components,i);
        update_component(component);
    }
}

void pe_element_set_material(PMaterial material){
  StaticMeshComponent* mesh = get_component_from_element(selected_element,STATIC_MESH_COMPONENT);
  mesh->material = material;
  //Model* model = array_get_last(&mesh->models_p);
  //model->material = material;

}

void pe_element_set_position(Element* element,vec3 position){

  TransformComponent *transform = pe_comp_get(TRASNFORM_COMPONENT);
  if (!transform)
    return;
  transform->model_matrix[3][0] = 0; 
  transform->model_matrix[3][1] = 0; 
  transform->model_matrix[3][2] = 0; 
  glm_translate(transform->model_matrix, position);
  glm_vec3_copy(transform->model_matrix[3], transform->position);
  
  for (int i = 0; i < selected_element->components.count; i++) {
    ComponentDefinition *component =
        array_get(&selected_element->components, i);
    update_component(component);
  }

}

void pe_element_duplicate(int current_count, Element *original) {
  Element *last_copy = selected_element;
  new_empty_element();
  for (int i = 0; i < last_copy->components.count; i++) {
    ComponentDefinition *component_definition =
        array_get(&last_copy->components, i);
    switch (component_definition->type) {
    case TRASNFORM_COMPONENT:
      add_transform_component_to_selected_element();
      TransformComponent *transform = pe_comp_get(TRASNFORM_COMPONENT);
      memcpy(transform, last_copy->transform, sizeof(TransformComponent));
      break;
    case CAMERA_COMPONENT:
      add_camera_component_to_selected_element();
      break;
    case STATIC_MESH_COMPONENT: {
      StaticMeshComponent *original_mesh =
          get_component_from_element(last_copy, STATIC_MESH_COMPONENT);
      
      StaticMeshComponent new_mesh;
      ZERO(new_mesh);

      array_new_pointer(&new_mesh.models_p,original_mesh->models_p.count);

      for (int i = 0; i < original_mesh->models_p.count; i++) {
        new_empty_model();
        PModel*original_model = array_get_pointer(&original_mesh->models_p,i);
        duplicate_model_data(selected_model, original_model);
        selected_model->shader_id = create_engine_shader(standart_vertex_shader,
                                                      standart_fragment_shader);
        array_add(&new_mesh.models_p, selected_model);
      }
      add_component_to_selected_element(sizeof(StaticMeshComponent), &new_mesh,
                                        STATIC_MESH_COMPONENT);
    } break;

    default:
      break;
    }
  }
}
void duplicate_selected_element(int current_count, Element* original){
    Element* last_copy = selected_element;
    new_empty_element();
    char new_name[30];
    sprintf(new_name,"%s%i",original->name,current_count);
    strcpy(selected_element->name,new_name);
    for(int i = 0; i < last_copy->components.count ; i++){
        ComponentDefinition* component_definition = array_get(&last_copy->components,i);
        switch (component_definition->type)
        {
        case TRASNFORM_COMPONENT:
            add_transform_component_to_selected_element();
            TransformComponent* transform = pe_comp_get(TRASNFORM_COMPONENT);
            memcpy(transform,last_copy->transform, sizeof(TransformComponent));
            break;
        case CAMERA_COMPONENT:
            add_camera_component_to_selected_element();
            break;
        case STATIC_MESH_COMPONENT:{
            StaticMeshComponent* original_mesh = get_component_from_element(last_copy,STATIC_MESH_COMPONENT);
            StaticMeshComponent new_mesh;
            memset(&new_mesh,0,sizeof(StaticMeshComponent));
            memcpy(&new_mesh,original_mesh,sizeof(StaticMeshComponent));
            memset(&new_mesh.meshes,0,sizeof(Array));
            array_init(&new_mesh.meshes,sizeof(u8),original_mesh->meshes.count);
            u8* model_id_in_loaded_model = array_get(&original_mesh->meshes,0);
            array_add(&new_mesh.meshes, model_id_in_loaded_model);
            for(int i = 1 ; i < original_mesh->meshes.count ; i++){
                new_empty_model();
                u8* original_model_id = array_get(&original_mesh->meshes,i);
                PModel* original_model = array_get(actual_model_array,*original_model_id);
                duplicate_model_data(selected_model,original_model);
                selected_model->shader_id = create_engine_shader(standart_vertex_shader,standart_fragment_shader);                
                u8 new_id = actual_model_array->count -1;
                array_add(&new_mesh.meshes, &new_id);
            }  
            add_component_to_selected_element(sizeof(StaticMeshComponent),&new_mesh,STATIC_MESH_COMPONENT);
        }          
        break;

        default:
            break;
        }
    }    
    

}
