#include "editor.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h> 

#include "engine/engine.h"

#include "text.h"

#include "engine/camera.h"

#include "engine/images.h"

#include "skeletal_editor.h"

#include "engine/level.h"

#include "geometry.h"

#include "Textures/texture_factory.h"

#include "HLOD/HLOD_factory.h"

#include "EditServer/edit_server.h"

#include "file_explorer.h"
#include "windows/content_browser.h"

#include "ProjectManager/project_manager.h"

#include "windows/windows.h"
#include "commands.h"
#include <engine/text_renderer.h>

#include "menus.h"

void play_game_standalone(){
    int exit_status = system("st sh ../level_editor/compile_game.sh");
}

void deselect_all(){
    for(int i = 0; i < editor_elements.count ; i++){
        Element* element = array_get(&editor_elements,i);
        element->selected = false;
    }
    current_component_selected = NULL;
}

void editor_message(const char* message){
    set_text_size(12);
    text_render(message , 0 + (-(camera_width_screen/2)) * pixel_size_x , 0 + (-(camera_heigth_screen/2)+12) * pixel_size_y  , pixel_size_x, pixel_size_y, false);   
}

void editor_add_HLOD_element(HLODCluster* cluster){
    
    new_empty_element();
    char name[20];
    sprintf(name,"HLODCluster_%i",cluster->id);
    strcpy(selected_element->name, name);              
    add_transform_component_to_selected_element();
    component_add_HLOD_to_select_element();
    HLODComponent* hlod = pe_comp_get(COMPONENT_HLOD);

    array_init(&hlod->childs,sizeof(void*),cluster->elements.count);
    for(int i = 0; i<cluster->elements.count; i++){
        Element** ppElement = array_get(&cluster->elements,i);        
        Element* element = ppElement[0];
        array_add(&hlod->childs, &element);
        element->proccess = false;
    }

    hlod->distance = 50;
    glm_aabb_center(hlod->bounding_box,hlod->center);
    hlod->has_childs_HLOD = false;

    char path[strlen(pavon_the_game_project_folder) + 40];
    sprintf(path,"%sContent/HLOD/HLOD_out%i.gltf",pavon_the_game_project_folder,cluster->id);
    pe_loader_model(path);
    Model* original = selected_model;
    new_empty_model();
    duplicate_model_data(selected_model, original);
    selected_model->shader = create_engine_shader(standart_vertex_shader,standart_fragment_shader);
    hlod->model = selected_model;
    Texture new_texture;
    sprintf(path,"HLOD/HLOD_texture%i.png",cluster->id);
    texture_load(path,&new_texture);
    hlod->model->texture.id = new_texture.id;
}

void editor_generate_and_add_cube_element(){
    new_empty_element();
    add_transform_component_to_selected_element();

    StaticMeshComponent mesh_component;    
    array_init(&mesh_component.meshes,sizeof(unsigned int),2);
    array_init(&mesh_component.textures,sizeof(unsigned int),2);
    int model_path_id = pe_arr_models_paths.count-1;
    array_add(&mesh_component.meshes,&model_path_id);

    new_empty_model();
    array_add(&mesh_component.meshes,&selected_model->id);

    selected_model->shader = create_engine_shader(standart_vertex_shader,standart_fragment_shader);
    geometry_cube_create_indices();
    vec3 box[2];
    glm_vec3_copy(VEC3(-1,-1,-1),box[0]);
    glm_vec3_copy(VEC3(1,1,1),box[1]);
    
    geometry_sphere_create_vertices(64);

    GPU_buffers_create_for_model(selected_model); 
    
    add_component_to_selected_element(sizeof(StaticMeshComponent),&mesh_component,STATIC_MESH_COMPONENT);
		LOG("Generated cubed added\n");

}

void add_editor_native_element(const char* native_element_name){
    
    if( strcmp("Camera", native_element_name) == 0 ){        
        new_empty_element();        
        strcpy(selected_element->name, "Camera01");              
        
        add_transform_component_to_selected_element();        

        CameraComponent camera_component;
        init_camera_component(&camera_component);
        add_component_to_selected_element(sizeof(CameraComponent), &camera_component, CAMERA_COMPONENT);
        
    }else if ( strcmp("Player Start", native_element_name) == 0 )
    {        
        new_empty_element();
        strcpy(selected_element->name, "PlayerStart01");        
              
        player_start = selected_element;
        add_transform_component_to_selected_element();        
        selected_model = NULL;
			
				PEComponentPlayerStart player_start_comp;
				ZERO(player_start_comp);
				add_component_to_selected_element(sizeof(PEComponentPlayerStart),&player_start_comp,PE_COMP_PLAYER_START);
						
    }else if ( strcmp("Player Controller", native_element_name) == 0 )
    {
        new_empty_element();
        strcpy(selected_element->name, "Player1");
        
    }else if ( strcmp("Sphere", native_element_name) == 0 ){
        new_empty_element();
        strcpy(selected_element->name, "Sphere");
        add_transform_component_to_selected_element();
        SphereComponent sphere;
        memset(&sphere,0,sizeof(SphereComponent));
        init_sphere_component(&sphere);
        add_component_to_selected_element(sizeof(SphereComponent), &sphere, SPHERE_COMPONENT);
    }
    else if ( strcmp("Cube", native_element_name) == 0 ){
        new_empty_element();
        strcpy(selected_element->name, "Cube");
        add_transform_component_to_selected_element();
        CubeComponent cube;
        memset(&cube,0,sizeof(SphereComponent));
        init_cube_component(&cube);
        add_component_to_selected_element(sizeof(CubeComponent), &cube, CUBE_COMPONENT);
    }else if ( strcmp("Empty Element", native_element_name) == 0 ){
        new_empty_element();
        strcpy(selected_element->name, "New Empty Element");
    }else if ( strcmp("Floor", native_element_name) == 0 ){
        new_empty_element();
        strcpy(selected_element->name, "Floor");
        add_transform_component_to_selected_element();
        StaticMeshComponent mesh_component;
        add_component_to_selected_element(sizeof(StaticMeshComponent),&mesh_component,STATIC_MESH_COMPONENT);

    }else if( strcmp("HLOD Cluster",native_element_name) == 0){
        new_empty_element();
        strcpy(selected_element->name, "HLOD Cluster");
        add_transform_component_to_selected_element();
        HLODBoxComponent box_component;
        memset(&box_component,0,sizeof(HLODBoxComponent));
        glm_vec3_copy(VEC3(1,1,1),box_component.bounding_box[0]);
        glm_vec3_copy(VEC3(-1,-1,-1),box_component.bounding_box[1]);
        add_component_to_selected_element(sizeof(HLODBoxComponent), &box_component, COMPONENT_HLOD_BOX);
        HLOD_clusters[HLOD_cluster_count] = selected_element;
        HLOD_cluster_count++;
    }
}


void editor_finish(){
    editor_running = false;
		pe_end(); 
		edit_server_finish();
	
}


void rotate_editor_element(Element* element, float angle, vec3 axis){
   TransformComponent* transform = get_component_from_element(element, TRASNFORM_COMPONENT);
   if(transform == NULL){
       LOG("No transfrom pointer in element\n");
       return;
   }
    versor new_rot_quat;
    glm_quatv(new_rot_quat, glm_rad(angle), axis);

    versor result_quat;
    glm_quat_mul(transform->rotation, new_rot_quat, result_quat);

    glm_quat_copy(result_quat, transform->rotation);

    mat4 model_rot_mat;
    glm_quat_mat4(new_rot_quat,model_rot_mat);

    glm_mul(transform->model_matrix,model_rot_mat, transform->model_matrix);

    for(int i = 0; i<selected_element->components.count; i++){
        ComponentDefinition* component = array_get(&selected_element->components,i);
        update_component(component);
    }
    
}

void rotate_editor_selected_element_with_quaternion(versor quaternion){
    mat4 model_rot_mat;
    glm_quat_mat4(quaternion,model_rot_mat);

    TransformComponent* transform = pe_comp_get(TRASNFORM_COMPONENT);
    if(transform)
        glm_mul(transform->model_matrix,model_rot_mat, transform->model_matrix);
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
                Model* original_model = array_get(actual_model_array,*original_model_id);
                duplicate_model_data(selected_model,original_model);
                selected_model->shader = create_engine_shader(standart_vertex_shader,standart_fragment_shader);                
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

void remove_selected_element(){
    selected_element = NULL;
    element_id_count--;
}

void reload_editor(){
    element_id_count = 0;
    
    array_clean(&editor_elements);   
    array_clean(&editor_models);
    array_clean(&pe_arr_models_paths);
    array_clean(&pe_arr_tex_paths);
    array_clean(actual_buttons_array);
    array_clean(&array_models_loaded);
}

void editor_update_command_queue(){

}

void editor_init_new_added_element(){
    
    strcpy(selected_element->name, "New Element");

    update_translation(main_camera.position);
    vec3 new_position;
    glm_vec3_scale(main_camera.front,4,new_position);
    update_translation(new_position);

    
    StaticMeshComponent* mesh_component = pe_comp_get(STATIC_MESH_COMPONENT); 
    if(!mesh_component)
        return;     
               
    for(int i = 1; i<=mesh_component->meshes.count-1 ; i++){                

        u8* id = array_get(&mesh_component->meshes,i);
        Model* model = array_get(actual_model_array,*id);
        model->texture.id = editor_texture_checker.id;

    }
}

void editor_add_element_with_model_path(const char* path){

    if(add_element_with_model_path(path) == -1)
			return;
    editor_init_new_added_element();
}


void editor_stats_calculates_triangles(){
    int triangles = 0;
    for(int i = 0; i<frame_draw_static_elements.count; i++){
        Model** modelp = array_get(&frame_draw_static_elements,i);
        if(!modelp)
            return;
				Model* model = modelp[0];
				if(!model)
					return;	
        int vertices_count = model->vertex_array.count;
        triangles += vertices_count/3;
    }
    for(int i = 0; i<frame_draw_skinned_elements.count; i++){
        Model** modelp = array_get(&frame_draw_static_elements,i);
        if(!modelp)
            return;
				Model* model = modelp[0];
				if(!model)
					return;	
        int vertices_count = model->vertex_array.count;
        triangles += vertices_count/3;
    }
	editor_stats_triangles = triangles;

}

void editor_focus_selected_element(){
    if(!selected_element)
        return;
    vec3 new_position;
    glm_vec3_copy(VEC3(selected_element->transform->position[0],
                    selected_element->transform->position[1] - 4,
                    selected_element->transform->position[2]),main_camera.position);
    vec3 direction;
    glm_vec3_sub(selected_element->transform->position, main_camera.position,direction);
    glm_normalize_to(direction,main_camera.front);
    camera_update(&current_window->camera);
}


void collision_test(){
    if(editor_sub_mode == EDITOR_SUB_MODE_TEXT_INPUT)
        return;
    if(input.C.pressed){

        Element* element1 = array_get(actual_elements_array,0);
        if(!element1)
            return;
        

        StaticMeshComponent* mesh = get_component_from_element(element1,STATIC_MESH_COMPONENT);
        unsigned int* modelid = array_get(&mesh->meshes,1);

        Element* element2 = array_get(actual_elements_array,1);
        if(!element2)
            return;
        StaticMeshComponent* mesh2 = get_component_from_element(element2,STATIC_MESH_COMPONENT);
        unsigned int* modelid2 = array_get(&mesh2->meshes,1);

        Model* model1 = array_get(actual_model_array,*modelid);
        Model* model2 = array_get(actual_model_array,*modelid2);
    
        if( collision_of(model1,model2) ){
            LOG("collision SAT\n");
            
        }
        else{
            LOG("NO SAT\n");
        }
    }


}


void editor_main_window_init(){
    window_create(window_editor_main, NULL, "editor"); 

    glfwSetKeyCallback(window_editor_main->window, &pe_input_key_callback);
		glfwSetCursorPosCallback(window_editor_main->window, &pe_input_mouse_movement_callback);
		glfwSetMouseButtonCallback(window_editor_main->window, &pe_input_mouse_button_callback);
    glfwSetCharCallback(window_editor_main->window, &pe_input_character_callback);
    glfwSetWindowFocusCallback(window_editor_main->window,&window_focus_callback);
    glfwSetFramebufferSizeCallback(window_editor_main->window, &window_resize_callback);

		window_editor_main->char_parser = pe_editor_parse_cmd_char;

    //draw_loading_screen();
    //glfwSwapBuffers(window_editor_main->window);    
		//
		
		
		//Load level form command line	
		if(strcmp(editor_level_open_path,"") != 0){
			editor_level_open(editor_level_open_path);
		}	
		
		
}

void editor_update(){

    editor_command_queue_udpate();    
    
    play_animation_list();

    //collision_test();    

}

void editor_render_init(){
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

    init_vec3(-6,0,2, main_camera.position);
    
		camera_update(&current_window->camera);

    editor_standard_fragment_shader = compile_shader(
        editor_standard_fragment_shader_source, GL_FRAGMENT_SHADER);


    load_model_to_array(&engine_native_models,
                        "/home/pavon/PavonEngine/NativeContent/Editor/sphere.glb",
                        "/home/pavon/PavonEngine/NativeContent/Editor/sphere_diffuse.png");
    load_model_to_array(&engine_native_models,
                        "/home/pavon/PavonEngine/NativeContent/Editor/cube.glb",
                        "/home/pavon/PavonEngine/NativeContent/Editor/cube_diffuse.jpg");
    load_model_to_array(&engine_native_models,
                        "/home/pavon/PavonEngine/NativeContent/Editor/camera.gltf",
                        "/home/pavon/PavonEngine/NativeContent/Editor/camera_gizmo.jpg");
    load_model_to_array(&engine_native_models,
                        "/home/pavon/PavonEngine/NativeContent/Editor/floor.glb",
                        "/home/pavon/PavonEngine/NativeContent/Editor/floor.jpg");
    texture_load("/home/pavon/PavonEngine/NativeContent/Editor/checker_texture.png",
                 &editor_texture_checker);

    editor_text_init();
   
	 	gizmos_init();
   
   	editor_running = true;
}

void editor_render_finish(){
	glfwTerminate();
}

void editor_draw() {

  glClearColor(COLOR(editor_background_color));
  render_clear_buffer(RENDER_COLOR_BUFFER | RENDER_DEPTH_BUFFER);

  text_draw_commands();

  if (isDrawUV)
    draw_UV();

  if (controlling_camera_component) {
    CameraComponent *camera =
        pe_comp_get(CAMERA_COMPONENT);
    update_main_camera_with_camera_component_values(camera);
  }

  for_each_element_components(&update_per_frame_component);

  test_elements_occlusion();
  check_meshes_distance();

  editor_stats_draw_calls = frame_draw_static_elements.count + frame_draw_skinned_elements.count;

  editor_stats_calculates_triangles();

  if (update_vertex_bones_gizmos)
    update_joints_vertex();

  engine_draw_elements(&frame_draw_static_elements);
	pe_render_skinned_elements(&array_skinned_mesh_pointers);

  pe_frame_clean();

  draw_gizmos();

  if (editor_mode == EDITOR_MODE_GUI_EDITOR ||
      editor_mode == EDITOR_PLAY_MODE) {
    draw_gui();
  }

  text_renderer_loop();
	pe_editor_menus_update();	
  // editor_message("editor message");
}

void editor_main_render_thread(){

	window_manager_draw_windows();

}

void editor_main_loop(){

        editor_update();    

}

void editor_data_init(){

    actual_model_array = &editor_models;
    actual_elements_array = &editor_elements;
    current_textures_array = &editor_textures;

    array_init(&selected_elements_id,sizeof(unsigned short int),100);
    array_init(&LOD_models,sizeof(Model),10);
    array_init(&editor_elements,sizeof(Element),100);
    array_init(&editor_models, sizeof(Model),100);
    array_init(&editor_textures, sizeof(Texture),100);    
    
    element_id_count = 0;    
    editor_mode = EDITOR_DEFAULT_MODE;
    editor_sub_mode = EDITOR_SUB_MODE_NULL;
    
    editor_sub_mode_text = "";

		strcpy(editor_mode_show_text,"Default");

    camera_velocity = 0.10;  
}

void editor_init() {
    pe_wm_renderer_type = PEWMOPENGLES2;
    pe_init();

    editor_data_init();

    pe_input_init();

    editor_command_queue_init();

    edit_server_init();

    render_thread_definition.init = &editor_render_init;
    render_thread_definition.draw = &editor_main_render_thread;
    render_thread_definition.end = &editor_finish;


    //All window definition here
    EngineWindow main_window;
    memset(&main_window, 0, sizeof(EngineWindow));
    main_window.init = &editor_main_window_init;//window specific data
    main_window.draw = &editor_draw;//Main loop draw in window
    main_window.finish = &editor_render_finish;
    main_window.input = &editor_window_level_editor_input_update;//handle editor modes 

    array_add(&engine_windows, &main_window);
    window_editor_main = array_pop(&engine_windows);

    //render thread initialization
    //Send window initialization to the render thread

    PEThreadCommand thread_commad;
    thread_commad.command = &window_manager_init_window;
    thread_commad.data = window_editor_main;
    thread_commad.done = false;
    thread_commad.type = POINTER;
    array_add(&render_thread_commads, &thread_commad);

    engine_init_render();

    //wait for window initialization in the render thread	
    while (!window_editor_main->initialized) {
    };


    LOG("[OK]Editor initialized\n");
}
