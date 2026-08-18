#include "LOD_system.h"
#include "../engine/engine.h"
#include "engine/camera.h"
void LOD_system_calculate_AABB_on_screen(StaticMeshComponent* mesh_component, vec2 box[2], vec2 center){
    mat4 projection_view;
    glm_mul(main_camera.projection,main_camera.view,projection_view);

    mat4 min_mat;
    glm_mat4_identity(min_mat);
    glm_translate(min_mat,mesh_component->bounding_box[0]);
    glm_mul(projection_view,min_mat, min_mat);
    mat4 max_mat;
    glm_mat4_identity(max_mat);
    glm_translate(min_mat,mesh_component->bounding_box[1]);
    glm_mul(projection_view,max_mat, max_mat);

    mat4 center_mat;
    glm_mat4_identity(center_mat);
    glm_translate(center_mat,mesh_component->center);
    glm_mul(projection_view,center_mat,center_mat);

    //Convert [-1,1] to [0,1]
        //Min - Max
    min_mat[3][0] = ( (min_mat[3][0] + 1.f) / 2.f ) * camera_width_screen;
    max_mat[3][0] = ( (max_mat[3][0] + 1.f) / 2.f ) * camera_width_screen;
    
    
    min_mat[3][1] = ( (1.f - min_mat[3][1]) / 2.f ) * camera_height_screen;
    max_mat[3][1] = ( (1.f - max_mat[3][1]) / 2.f ) * camera_height_screen;


        //Center
    center_mat[3][0] = ( (center_mat[3][0] + 1.f) / 2.f ) * camera_width_screen;
    center_mat[3][1] = ( (1.f - center_mat[3][1]) / 2.f ) * camera_height_screen;



    box[0][0] = min_mat[3][0];
    box[0][1] = min_mat[3][1];

    box[1][0] = max_mat[3][0];
    box[1][1] = max_mat[3][1];
    

    center[0] = center_mat[3][0];
    center[1] = center_mat[3][1];
}



void LOD_check_distance_static_mesh_component_and_add_to_draw_elements(StaticMeshComponent* static_mesh_component){
    float distance = glm_vec3_distance(main_camera.position,static_mesh_component->center);

    vec2 aabb[2];
    vec2 center;
    LOD_system_calculate_AABB_on_screen(static_mesh_component,aabb,center);
    DrawData new_draw;
    new_draw.vertex = UI_plane_vertex_buffer_id;
    PModel* model = array_get(actual_model_array,0);
    if(!model)
        return;
    new_draw.shader = model->shader_id;
    //draw_two_dimention_element(&new_draw,center,(vec2){20,20},(vec4){1,0,0,1});
    
    float distaces[3] = {0,24,40};

    u8 id;
    int count = static_mesh_component->meshes.count-1;
    for(int i = 1; i <= count ; i++){
        float distance_value = distaces[i-1];            
        if(distance_value==0){
            u8* model_id = array_get(&static_mesh_component->meshes,i);
            id = *model_id;
            continue;
        }
        float next_distace_value = distance_value+1;
        if(i != count)
            next_distace_value = distaces[i];
            
        if(distance >= distance_value && distance_value < next_distace_value){
            u8* model_id = array_get(&static_mesh_component->meshes,i);
            id = *model_id;            
        }
        
    }
    PModel* draw_model = array_get(actual_model_array,id);
    
    array_add(&frame_draw_static_elements,&draw_model);
}  
