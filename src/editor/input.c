#include "input.h"

#include "../engine/utils.h"


#include <stdlib.h>

#include "../engine/engine.h"


#include "editor.h"
#include "../engine/game.h"

#include "editor_mode.h"
#include "../engine/gui.h"

#include "commands.h"
#include "skeletal_editor.h"

#include <engine/audio/audio_engine.h>


#include "Blender/blender.h"

#include <editor/modeling.h>

float rotate_value = 1;
float camera_rotate_yaw = 1;

vec3 rotate_direction = (vec3){0,0,1};

float last_mouse_x = 400;
float last_mouse_y = 300;
bool first_mouse_movement = true;


//grab mode
float move_object_value = 0.5;
float scale_object_value= 0.01;
bool grid_translate = false;
vec2 move_ui_element_value;//per pixel
float move_ui_element_value_per_axis = 0.6;

bool player_in_start_position = false;

void input_change_mode() {

  if (key_released(&input.G)) {
    change_to_editor_sub_mode(EDITOR_SUB_MODE_GRAB);
  }

  if (key_released(&input.R)) {
    change_to_editor_sub_mode(EDITOR_SUB_MODE_ROTATE);
  }
  
  if (key_released(&input.V)) {
    change_to_editor_mode(EDITOR_NAVIGATE_MODE);
  }
  
  if (key_released(&input.ESC)) {
    change_to_editor_mode(EDITOR_DEFAULT_MODE);
  }
  
}

bool editor_input_camera_rotate_control() {
  float rotate_velocity = 200;
  bool rotate_keys_pressed = false;
  bool rotate_yaw_keys_pressed = false;
  if (input.J.pressed) {
    rotate_keys_pressed = true;
    rotate_value -= rotate_velocity * time_delta;
    if (rotate_value < -10000) {
      rotate_value = -100;
    }
  }

  if (input.K.pressed) {
    rotate_keys_pressed = true;
    rotate_value += rotate_velocity * time_delta;
    if (rotate_value > 10000) {
      rotate_value = 100;
    }
  }

         
  if (input.M.pressed) {
    rotate_yaw_keys_pressed= true;
    camera_rotate_yaw += rotate_velocity * time_delta;
    if (rotate_value > 10000) {
      camera_rotate_yaw = 100;
    }
    }
  if (input.N.pressed) {
    rotate_yaw_keys_pressed= true;
    camera_rotate_yaw -= rotate_velocity * time_delta;
    if (rotate_value < -10000) {
      camera_rotate_yaw = -100;
    }
    }

  if (rotate_yaw_keys_pressed== true) {

    verticalAngle += 600 / 2 - camera_rotate_yaw;

    verticalAngle *= 0.05;

    camera_rotate_control(verticalAngle, 0);
    return true;
  }

  if (rotate_keys_pressed) {

    horizontalAngle += camera_width_screen / 2 - rotate_value;

    verticalAngle += 600 / 2 - camera_rotate_yaw;

    horizontalAngle *= 0.05;
    verticalAngle *= 0.05;

    camera_rotate_control(0, horizontalAngle);
    return true;
  }
  return false;
}

void editor_input_navigate() {
  input_change_mode();
  float camera_navigate_velocity = 0.05f;
  if (input.I.pressed) {
    camera_velocity += camera_navigate_velocity;
  }
  if (input.O.pressed) {
    camera_velocity -= camera_navigate_velocity;
  }

  vec3 velocity_vector;
  float velocity_per_frame = camera_velocity * time_delta;
  glm_vec3_copy(
      (vec3){velocity_per_frame, velocity_per_frame, velocity_per_frame},
      velocity_vector);
  bool update = false;

  if (input.E.pressed) {
    vec3 move;
    glm_vec3_mul(velocity_vector, main_camera.up, move);
    glm_vec3_add(main_camera.position, move, main_camera.position);
    update = true;
  }
  if (input.Q.pressed) {
    vec3 move;
    glm_vec3_mul(velocity_vector, main_camera.up, move);
    glm_vec3_sub(main_camera.position, move, main_camera.position);
    update = true;
  }

  if (input.W.pressed) {
    vec3 move;
    glm_vec3_mul(velocity_vector, main_camera.front, move);
    glm_vec3_add(main_camera.position, move, main_camera.position);
    update = true;
  }
  if (input.S.pressed) {
    vec3 move;
    glm_vec3_mul(velocity_vector, main_camera.front, move);
    glm_vec3_sub(main_camera.position, move, main_camera.position);
    update = true;
  }
  if (input.D.pressed) {
    vec3 cross;
    glm_vec3_cross(main_camera.front, main_camera.up, cross);
    glm_normalize(cross);
    vec3 move;
    glm_vec3_mul(velocity_vector, cross, move);
    glm_vec3_add(main_camera.position, move, main_camera.position);
    update = true;
  }
  if (input.A.pressed) {
    vec3 cross;
    glm_vec3_cross(main_camera.front, main_camera.up, cross);
    glm_normalize(cross);
    vec3 move;
    glm_vec3_mul(velocity_vector, cross, move);
    glm_vec3_sub(main_camera.position, move, main_camera.position);
    update = true;
  }

  if (editor_input_camera_rotate_control()) {
    camera_update(&main_camera);
  }
  if (update)
    camera_update(&main_camera);
}

void grab_mode(){
    draw_translate_gizmo = true;
    draw_rotate_gizmo = false;
    gizmos_draw_scale = false;

    if(key_released(&input.KEY_1)){
        grid_translate = true;
    }
    if(!grid_translate){
        if(key_released(&input.I)){
            if(editor_mode == EDITOR_DEFAULT_MODE)
                move_object_value += 0.00005;

            if(editor_mode == EDITOR_MODE_GUI_EDITOR)
                move_ui_element_value_per_axis += 0.1;
        }
        if(key_released(&input.O)){
            if(editor_mode == EDITOR_DEFAULT_MODE)
                move_object_value -= 0.00005;

            if(editor_mode == EDITOR_MODE_GUI_EDITOR)
                move_ui_element_value_per_axis -= 0.1;
        }
    }
    
    input_change_mode();
        

    if(editor_mode == EDITOR_DEFAULT_MODE){
        editor_input_camera_rotate_control();     
        if(input.SHIFT.pressed){
            editor_input_navigate();
        }
        vec3 move;
        bool update = false;
        bool camera_space = true;
        if(!grid_translate){
            if(camera_space){
                vec3 velocity_vector;
                glm_vec3_copy(VEC3(move_object_value,move_object_value,move_object_value) , velocity_vector);
                if(input.D.pressed){                    
                    vec3 cross;
                    glm_vec3_crossn(main_camera.front, main_camera.up, cross);                    
                    glm_vec3_mul(velocity_vector, cross , move );
                    update = true;
                }else if(input.A.pressed){
                    vec3 cross;
                    glm_vec3_crossn(main_camera.front, main_camera.up, cross);
                    glm_vec3_inv(velocity_vector); 
                    glm_vec3_mul(velocity_vector, cross , move );
                    update = true;
                }else if(input.W.pressed){
                    glm_vec3_mul(velocity_vector,main_camera.front,move);
                    update = true;
                }else if(input.S.pressed){
                    glm_vec3_inv(velocity_vector);
                    glm_vec3_mul(velocity_vector,main_camera.front,move);
                    update = true;
                }else if(input.E.pressed){
                    glm_vec3_mul(velocity_vector,main_camera.up,move);
                    update = true;
                }else if(input.Q.pressed){
                    glm_vec3_inv(velocity_vector);
                    glm_vec3_mul(velocity_vector,main_camera.up,move);
                    update = true;
                }                                
            }else{//World Transform
                if(input.W.pressed){
                    glm_vec3_copy( VEC3(0,-move_object_value,0) , move );
                    update = true;
                }
                if(input.S.pressed){
                    glm_vec3_copy( VEC3(0,move_object_value,0) , move );
                    update = true;            }
                if(input.D.pressed){
                    glm_vec3_copy( VEC3(-move_object_value,0,0) , move );                
                    update = true;            
                }
                if(input.A.pressed){
                    glm_vec3_copy( VEC3(move_object_value,0,0) , move );
                    update = true;            
                }
                if(input.E.pressed){
                    glm_vec3_copy( VEC3(0,0,move_object_value) , move );
                    update = true;            
                }
                if(input.Q.pressed){
                    glm_vec3_copy( VEC3(0,0,-move_object_value) , move );
                    update = true;            
                }
            }           
            
            if(current_component_selected != NULL){
                if(update){
                    CameraComponent* camera = current_component_selected->data;
                    glm_vec3_add(camera->position,move,camera->position);
                }              
            }else
            {
                if(update)
                    update_translation(move);
            }            
            
            
        }else{//grid translate
            if(key_released(&input.KEY_1)){
                grid_translate = false;
            } 
            
            if(key_released(&input.D)){
                vec3 move = {move_object_value,0,0};
                update_translation(move);
            }
            if(key_released(&input.A)){
                vec3 move = {-move_object_value,0,0};
                update_translation(move);
            }
            if(key_released(&input.S)){
                vec3 move = {0,-move_object_value,0};
                update_translation(move);
            }
            if(key_released(&input.W)){
                vec3 move = {0,move_object_value,0};
                update_translation(move);
            }
            if(key_released(&input.I)){
                move_object_value += 2.4;
            }
            if(key_released(&input.O)){
                move_object_value = 24;
            }
        }
    }

    if(editor_mode == EDITOR_MODE_GUI_EDITOR){
        if(selected_button == NULL)
            return;
        if(input.W.pressed){
            move_ui_element_value[0] = 0;
            move_ui_element_value[1] = move_ui_element_value_per_axis;
            glm_vec3_sub(selected_button->position, move_ui_element_value, selected_button->position);
        }
        if(input.S.pressed){
            move_ui_element_value[0] = 0;
            move_ui_element_value[1] = move_ui_element_value_per_axis;            
            glm_vec3_add(selected_button->position, move_ui_element_value, selected_button->position);
        }
        if(input.D.pressed){
            move_ui_element_value[1] = 0;
            move_ui_element_value[0] = move_ui_element_value_per_axis;             
            glm_vec3_add(selected_button->position, move_ui_element_value, selected_button->position);
        }
        if(input.A.pressed){
            move_ui_element_value[1] = 0;
            move_ui_element_value[0] = move_ui_element_value_per_axis;            
            glm_vec3_sub(selected_button->position, move_ui_element_value, selected_button->position);
        }
    }
    
}

void scale_mode() {

  input_change_mode();

  gizmos_draw_scale = true;

  if (editor_mode == EDITOR_DEFAULT_MODE) {

    vec3 move;
    bool update = false;

    if (input.J.pressed) {
      glm_vec3_copy(VEC3(scale_object_value,
					 	scale_object_value,
					 	scale_object_value), move);
      update = true;
    }
    if (input.K.pressed) {
      glm_vec3_copy(VEC3(-scale_object_value, 
						-scale_object_value, 
						-scale_object_value), move);
      update = true;
    }
    if (input.D.pressed) {
      glm_vec3_copy(VEC3(-scale_object_value, 0, 0), move);
      update = true;
    }
    if (input.A.pressed) {
      glm_vec3_copy(VEC3(scale_object_value, 0, 0), move);
      update = true;
    }
    if (input.E.pressed) {
      glm_vec3_copy(VEC3(0, 0, scale_object_value), move);
      update = true;
    }
    if (input.Q.pressed) {
      glm_vec3_copy(VEC3(0, 0, -scale_object_value), move);
      update = true;
    }

    if (update)
      update_scale(move);
  }
}

void pe_editor_input_modeling_mode(){
  
  draw_rotate_gizmo = false;
  draw_translate_gizmo = false;
  gizmos_draw_scale = false;
  
  input_change_mode();

	if(key_released(&input.A)){
		vertex_new(0,0,0)	;
	}
	
  if(key_released(&input.D)){
		vertex_new(0.1,0.1,0.1)	;
	}
	
  if(key_released(&input.K)){

		vertex_new(1,1,1)	;

	}

  if(key_released(&input.E)){
    
      draw_translate_gizmo = true; 
      pe_modeling_extrude_vertex(NULL, NULL)    ;
      change_to_editor_sub_mode(EDITOR_SUB_MODE_GRAB);

	}


  if(key_released(&input.U)){
    pe_modeling_select_next_vertex();
  }

	draw_vertices();


}


void default_mode() {
  if (editor_sub_mode == EDITOR_SUB_MODE_TEXT_INPUT)
    return;

  draw_rotate_gizmo = false;
  draw_translate_gizmo = false;
  gizmos_draw_scale = false;

  input_change_mode();

  if(key_released(&input.SEMICOLON)){
      pe_editor_parse_cmd_char(':');
  }
  if (editor_sub_mode == EDITOR_SUB_MODE_TEXT_INPUT) {

  }

  if (key_released(&input.TAB)) {
    change_to_editor_mode(PE_EDITOR_MODELING_MODE);
  }

  if (selected_element != NULL) {
    if (key_released(&input.S)) {
      change_to_editor_sub_mode(EDITOR_SUB_MODE_SCALE);
    }
  }
  if(key_released(&input.Q)){
    LOG("Exit");
    exit(0);
  }
  if (input.B.pressed) {
    if (audio2.playing == false) {
      array_add_pointer(&pe_audio_array_queue, &audio2);
      LOG("play cat\n");
      audio2.playing = true;
      audio2.finish = false;
      audio2.file.bytes_readed = sizeof(struct PWaveHeader);
    }
  }

  // edit in blender
  if (key_released(&input.TAB)) {
    if (selected_element) {
      if (selected_element->editor_data.has_blend_file) {
        char relative_path[] = "../assets/";
        char new_file_path[strlen(relative_path) +
                           strlen(
                               selected_element->editor_data.blend_file_path)];
        memset(new_file_path, 0, strlen(new_file_path));
        strcat(new_file_path, relative_path);
        strcat(new_file_path, selected_element->editor_data.blend_file_path);
        system_command("blender ", new_file_path);
        strcpy(editing_blender_file_path, new_file_path);
        is_editing_blender_file = true;
      }
    }
  }

// #ifdef DESKTOP
//   if (key__released(&input.D, GLFW_MOD_SHIFT)) {
//     duplicate_selected_element(1, selected_element);
//     LOG("duplicated \n");
//     return;
//   }
//   if (key__released(&input.A, GLFW_MOD_ALT)) {
//     LOG("deselect all \n");
//     deselect_all();
//     return;
//   }
//
//   if (key__released(&input.P, GLFW_MOD_SHIFT)) {
//     LOG("editor play mode \n");
//     change_to_editor_mode(EDITOR_PLAY_MODE);
//     return;
//   }
//
//   if (key__released(&input.P, GLFW_MOD_CONTROL)) {
//     // TODO: open game in new window
//     LOG("playing \n");
//     play_game_standalone();
//     return;
//   }
// #endif

  if (key_released(&input.X)) {
    remove_selected_element();
    return;
  }

  if (key_released(&input.F)) {
    editor_focus_selected_element();
  }

  if (key_released(&input.KEY_1)) {
    if (can_draw_gizmos) {
      can_draw_gizmos = false;
    } else
      can_draw_gizmos = true;
    return;
  }
  if (key_released(&input.KEY_2)) {

    init_skeletal_editor();

    can_draw_skeletal_bones = true;
    return;
  }
  if (key_released(&input.KEY_3)) {
    LOG("Draw bounding box\n");
    can_draw_bounding_box_in_select_element = true;
    return;
  }

  if (key_released(&input.Q)) {
    if (controlling_camera_component) {
      controlling_camera_component = false;
      memcpy(&main_camera, &saved_camera, sizeof(CameraComponent));
      camera_update(&main_camera);
      return;
    }
    CameraComponent *camera_component = pe_comp_get(CAMERA_COMPONENT);
    if (camera_component) {
      controlling_camera_component = true;
      change_view_to_camera_component(camera_component);
    }
  }
  if (key_released(&input.M)) {
    editor_file_explorer_show = true;
  }
  if (key_released(&input.A)) {
    editor_content_browser_show = true;
  }
}

void rotate_input_mode(){
    input_change_mode();

    draw_rotate_gizmo = true;
    draw_translate_gizmo = false;
    gizmos_draw_scale = false;

		bool can_rotate = false;

    if(key_released(&input.R)){
        change_to_editor_mode(EDITOR_DEFAULT_MODE);
        return;
    }

    if(key_released(&input.X)){
				glm_vec3(VEC3(1,0,0),rotate_direction);
    }

    if(key_released(&input.Y)){
				glm_vec3(VEC3(0,1,0),rotate_direction);
    }
    if(key_released(&input.Z)){
				glm_vec3(VEC3(0,0,1),rotate_direction);
    }

    if (input.SHIFT.pressed) {
      rotate_value = 0.1f;
    } else {
      rotate_value = 1;
    }

    if (input.J.pressed) {
      rotate_value = -rotate_value;
			can_rotate = true;
    }
    if (input.K.pressed) {
      rotate_value = rotate_value;
			can_rotate = true;
    }
		if(can_rotate)
			rotate_editor_element(selected_element, rotate_value, rotate_direction);
}


void input_mode_play(){
  
    //update_game();
    if(loop_fuction_dynamic_loaded)
        loop_fuction_dynamic_loaded(&input);

    controlling_camera_component = true;

    if(key_released(&input.ESC)){

        player_in_start_position = false;
        controlling_camera_component = false;
        loaded_gamplay_library = false;
        loop_fuction_dynamic_loaded = NULL;
        close_dynamic_game_play();
        change_to_editor_mode(EDITOR_DEFAULT_MODE);
        return;
    }
    if(!player_in_start_position){
        if(player1 != NULL){
            if(player_start == NULL){
                //LOG("No player start\n");
                //return;
            }            

            //set_element_position(player1,player_start->transform->position);
            player_in_start_position = true;
        }else{
            //LOG("No player selected\n");
           // return;
        }
    }
    
}

void input_gui_editor(){
    if(editor_sub_mode != EDITOR_SUB_MODE_NULL)
        return;
    
    if(key_released(&input.ESC)){
        change_to_editor_mode(EDITOR_DEFAULT_MODE);
        //array_clean(actual_buttons_array);
    }
        
// #ifdef DESKTOP 
//     if(key__released(&input.A,GLFW_MOD_SHIFT)){
//         new_empty_button();
//     }
// #endif

    if(key_released(&input.G)){
        change_to_editor_sub_mode(EDITOR_SUB_MODE_GRAB);
    }
}

void editor_window_level_editor_input_update(){
   //Editor Normal Mode 
	if(editor_sub_mode == EDITOR_SUB_MODE_NULL){
        switch (editor_mode)
        {    
        case EDITOR_DEFAULT_MODE:
            default_mode();
            break;
        case EDITOR_NAVIGATE_MODE:
            editor_input_navigate();
            break;        
        case PE_EDITOR_MODELING_MODE:
            pe_editor_input_modeling_mode();
            break;        
        case EDITOR_PLAY_MODE:
            input_mode_play();
            break;
        case EDITOR_MODE_GUI_EDITOR:
            input_gui_editor();
            break;
        case EDITOR_CHANGING_MODE_MODE:
            editor_mode = mode_to_change;
            break;
        default:
            break;
        } 

    }
   //Editor Sub Mode 
    switch (editor_sub_mode)
    {
    case EDITOR_SUB_MODE_TEXT_INPUT:
        text_input_mode();
        break;
    case EDITOR_SUB_MODE_GRAB:
        grab_mode();            
        break;
    case EDITOR_SUB_MODE_SCALE:
        scale_mode();
        break;
    case EDITOR_SUB_MODE_ROTATE:
        rotate_input_mode();
        break;
    case EDITOR_SUB_MODE_NULL:
        break;

    default:
        break;
    }
   
}
