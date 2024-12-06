#include "content_browser.h"
#include "../editor.h"
#include "../Textures/texture_factory.h"
#include "../text.h"


#include "../../engine/gui.h"

#include <editor/project_manager.h>
#include "../editor_shader.h"

#include "../commands.h"

#include "../content_manager.h"
#include "../../engine/content_manager.h"
 

PModel content_model;

Array array_content_views;
Array array_finding_content;

ContentView* editor_content_view_found = NULL;

void editor_window_content_browser_draw_content_view(ContentView* content_view){

  //draw_two_dimention_element(&content_view->draw, content_view->position, content_view->size, (vec4){1,0,1,1});
	text_render_in_screen_space(content_view->text_size,content_view->content_name,content_view->position[0]-64,-content_view->position[1]-64);

}

void content_find_per_name(const char* name){
    for (int i = 0; i < array_content_views.count; i++)
    {
        bool found = false;
        ContentView* content_view = array_get(&array_content_views,i);

        if(content_view->content_name[0] != name[0])
            continue;

        for (u8 j = 0; j < strlen(name); j++)
        {
            if(content_view->content_name[j] != name[j]){
                found = false;
                break;
            }

            found = true;
        }
        if(found)
        array_add(&array_finding_content,&content_view);
    }
}


void editor_window_content_browser_search_mode(){
    array_clean(&array_finding_content);
    content_find_per_name(&command_text_buffer[1]);

    for (int i = 0; i < array_finding_content.count; i++)
    {
        ContentView** ppContent_view = array_get(&array_finding_content,i);
        ContentView* content_view = ppContent_view[0];
        editor_window_content_browser_draw_content_view(content_view);
    }
}


void selection_create_hint(struct Hint* out){
    int hint_count = array_content_views.count;
    int max_hint_key_count = 4;
    char available_keys[] = "jhdgksl";
    int divided = hint_count / max_hint_key_count;

    int key_letters_count = 0;
    
    char current_key = 'j';
   
    for (int i = 0; i < hint_count; i++)
    {
        struct Hint new_hint;
        memset(&new_hint,0,sizeof(struct Hint));

        if(key_letters_count < strlen(available_keys)){
            current_key = available_keys[key_letters_count];
            new_hint.keys[0] = current_key;
            key_letters_count++;
        }else{
            current_key = available_keys[key_letters_count];
            key_letters_count = 0;
           
        }

        
        out[i] = new_hint;
    }    
    key_letters_count = 1;

    /* Start in one becouse the first letter has already been assigned */
    for (u8 j = 1; j < divided; j++)
    {
        for (int i = 0; i < hint_count; i++)
        {
            struct Hint* hint = &out[i];

            if(key_letters_count < strlen(available_keys)){
                current_key = available_keys[key_letters_count];
                hint->keys[j] = current_key;
                key_letters_count++;
            }else{
                current_key = available_keys[key_letters_count];
                key_letters_count = 1;
                
            }
        }
    }

}

void editor_window_content_add_content_render_thread(){
	window_set_focus(window_editor_main);               	
	char directory[sizeof(project_manager_current_path) + 34];
	sprintf(directory,"%s%s%s%s",project_manager_current_path,content_folder,editor_content_view_found->content_name,".pb");               

	ContentType type = content_manager_load_content(directory);
    switch (type)
    {
    case CONTENT_TYPE_STATIC_MESH:{
        
		editor_init_new_added_element();
        break;
    }
   	case CONTENT_TYPE_TEXTURE:{
		break;


	} 
    default:
        break;
    }
}

void editor_window_content_browser_input_update(){
  if (key_released(&input.F)) {
    if (editor_window_content_browser_hint) {
      editor_window_content_browser_hint = false;
    } else {
      editor_window_content_browser_hint = true;
      LOG("Hint\n");
    }
    return;
  }
#ifdef DESKTOP
  if (key__released(&input.ENTER, GLFW_MOD_SHIFT)) {
    LOG("Import\n");
    memset(command_text_buffer, 0, sizeof(command_text_buffer));
    command_character_count = 0;
    editor_window_content_browser_hint = false;
    if (editor_content_view_found) {
      // need to send command to render thread
      ExecuteCommand new_command;
      new_command.executed = false;
      new_command.command = editor_window_content_add_content_render_thread;
      array_add(&array_render_thread_commands, &new_command);
      return;
    } else {
      LOG("No content selected\n");
    }
  }
#endif
  if (key_released(&input.ENTER)) { // select content
    change_to_editor_sub_mode(EDITOR_SUB_MODE_NULL);
    memset(command_text_buffer, 0, sizeof(command_text_buffer));
    command_character_count = 0;
    editor_window_content_browser_hint = false;
    LOG("Content selected\n");
    return;
  }

  if (key_released(&input.E)) {
    if (!editor_content_view_found)
      return;
    LOG("Edit Mesh: %s\n", editor_content_view_found->content_name);
    tabs_new(current_window, editor_content_view_found->content_name);
    current_window->tab_current->draw = editor_window_static_mesh_draw;
    return;
  }
}

void editor_content_draw_type_in_text(ContentView *content_view) {

  switch (content_view->type) {
  case CONTENT_TYPE_TEXTURE: {

    text_render_in_screen_space(12, "Texture", content_view->position[0] - 58,
                                -content_view->position[1] - (64 - 120));
    break;
  }
  case CONTENT_TYPE_STATIC_MESH: {

    text_render_in_screen_space(12, "Static Mesh",
                                content_view->position[0] - 58,
                                -content_view->position[1] - (64 - 100));
    break;
  }
	case CONTENT_TYPE_LEVEL: {

    text_render_in_screen_space(12, "Level",
                                content_view->position[0] - 58,
                                -content_view->position[1] - (64 - 100));
													 }
													 break;
  }
}

void editor_window_content_browser_draw() {
  // if in new window clean screen first
  // glClearColor(0.1,0.2,0.4,1);
  // glClear(GL_DEPTH_BUFFER_BIT);

  if (editor_search_objects) {
    editor_window_content_browser_search_mode();

  } else {

    ContentView *mark_content = array_get(&array_content_views, 0);
    if (mark_content)
      mark_content->selected = true;

    // Draw in screen contents
    for (int i = 0; i < array_content_views.count; i++) {
      ContentView *content_view = array_get(&array_content_views, i);
      if (!content_view)
        continue;
      editor_content_draw_type_in_text(content_view);
      editor_window_content_browser_draw_content_view(content_view);
    }

    if (editor_window_content_browser_hint) { // hint activated
      change_to_editor_sub_mode(EDITOR_SUB_MODE_TEXT_INPUT);

      struct Hint hints[array_content_views.count];
      selection_create_hint(hints);

      for (int i = 0; i < array_content_views.count; i++) {
        ContentView *content_view = array_get(&array_content_views, i);
        if (!content_view)
          continue;

        vec2 hint_position;
        vec2 hint_size;
        hint_position[0] = content_view->position[0] + 35;
        hint_position[1] = content_view->position[1];
        hint_size[0] = 20;
        hint_size[1] = 20;

        // draw_two_dimention_element(&content_view->draw,hint_position,hint_size,(vec4){0,1,0,1});
        text_render_in_screen_space(12, hints[i].keys, hint_position[0],
                                    -hint_position[1]);
      }

      // Input compare
      if (strlen(command_text_buffer) >= 1) {
        int count_found = 0;
        bool found = false;
        for (u8 i = 0; i < array_content_views.count; i++) {
          if (found)
            break;

          struct Hint hint = hints[i];

          for (u8 j = 0; j < strlen(command_text_buffer); j++) {

            for (u8 k = 0; k < strlen(hint.keys); k++) {

              if (command_text_buffer[k] != hint.keys[k]) {
                found = false;
                break;
              }
              found = true;
            }
            if (found) {
              editor_content_view_found = array_get(&array_content_views, i);
              if (!editor_content_view_found)
                continue;

              LOG("%s\n", editor_content_view_found->content_name);
            }
          }
        }
      }
    }
  }

  if (editor_sub_mode == EDITOR_SUB_MODE_TEXT_INPUT) {
    set_text_size(12);
    text_render(command_text_buffer,
                0 + (-(camera_width_screen / 2)) * pixel_size_x,
                0 + (-(camera_heigth_screen / 2) + 24) * pixel_size_y,
                pixel_size_x, pixel_size_y, false);
  }
}

ContentView first;

void content_view_create_model_view(int image_size){
#ifdef DESKTOP

    glClearColor(1,0.2,0.4,1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw_simgle_model(selected_model);
#endif
}

const char* content_manager_current_content_path;
ContentType content_manager_current_content_type;
void content_create_draw_image_thumbnail(int size){
#ifdef DESKTOP
#endif
    glClearColor(1,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    PTexture texture;
    pe_load_texture(content_manager_current_content_path,&texture);

    DrawData data;
    data.shader = create_engine_shader(standart_vertex_shader,editor_standard_fragment_shader);
    data.vertex = UI_plane_vertex_buffer_id;
    data.texture = texture.id;
    camera_heigth_screen = 128;
    camera_width_screen  = 128;
    pe_render_2d(&data, (vec2){64,64}, (vec2){64,64}, (vec4){1,1,1,1});
    camera_heigth_screen = 720;
    camera_width_screen = 1280;
}

bool content_thumbnail_created = false;
void content_manager_render_threar_render_to_texture(){
		render_to_texture(128,content_create_draw_image_thumbnail);
		char directory[sizeof(pavon_the_game_project_folder) + 150];
		memset(directory,0,sizeof(directory));
		sprintf(directory,"%s%s%s%i%s",pavon_the_game_project_folder,".thumbnails/","texute",01,".png");
		texture_current_export_name = directory;
		//texture_export(128);
		content_thumbnail_created = true;
}

void content_create_thumbnail(const char * brute_content_path,ContentType type){
    content_manager_current_content_path = brute_content_path;
    content_manager_current_content_type = type;
    switch (type)
    {
    case CONTENT_TYPE_TEXTURE:
        {
			ExecuteCommand new_command;
			new_command.executed = false;
			new_command.command = &content_manager_render_threar_render_to_texture;
//			array_add(&array_render_thread_commands,&new_command);
			//while(!content_thumbnail_created)
			content_manager_render_threar_render_to_texture();
            break;
        }
    
    default:
        break;
    }

}

void content_browser_window_create_contents_thumbnails(){

    int memory_mark = engine_memory_mark();
    Array models_loaded_for_create_thumbnails;
    array_init(&models_loaded_for_create_thumbnails,sizeof(PModel),array_content_views.count+5);
    Array* prev_model_array = actual_model_array;
    actual_model_array = &models_loaded_for_create_thumbnails;
    int model_offset = 0;   

    memcpy(&saved_camera,&main_camera,sizeof(CameraComponent));

    for (int i = 0; i < array_content_views.count; i++)
    {
        ContentView* content_view = array_get(&array_content_views,i);
        if(!content_view)
            continue;
        char directory[sizeof(pavon_the_game_project_folder) + 150];
        sprintf(directory,"%s%s%s",pavon_the_game_project_folder,"Content/",content_view->content_name);
        int models_count = pe_loader_model(directory);
        PModel* model = array_get(&models_loaded_for_create_thumbnails,i+model_offset);
        model_offset = 0;
        if(models_count > 1){
            
            model_offset ++;
            
        }

        model->shader = create_engine_shader(standart_vertex_shader,standart_fragment_shader);

        selected_model = model;
        selected_model->texture.id = editor_texture_checker.id;

        vec3 center;
        vec3 aabb[2];
        glm_vec3_copy(selected_model->min,aabb[0]);
        glm_vec3_copy(selected_model->max,aabb[1]);
        glm_aabb_center(aabb,center);
        
        glm_vec3_copy(VEC3(0,0,0),main_camera.position);

        vec3 frustrum_box[2];
 /*        while ( !glm_aabb_contains(frustrum_box,aabb))
        {
            mat4 inverse_mat;
            mat4 view_projection;
            glm_mul(main_camera.projection,main_camera.view,view_projection);
            glm_mat4_inv(view_projection,inverse_mat);

            vec4 corners[8];
            glm_frustum_corners(inverse_mat,corners);
            
            glm_frustum_box(corners,main_camera.view,frustrum_box);
            glm_vec3_add(main_camera.position,VEC3(0,0.1,0),main_camera.position);
            glm_lookat(main_camera.position, center, main_camera.up , main_camera.view);  
        } */
        
        glm_vec3_sub(main_camera.position,center,main_camera.front);
        glm_vec3_norm(main_camera.front);

        glm_vec3_add( VEC3(0,5,0) ,main_camera.position, main_camera.position );
        glm_lookat(main_camera.position, center, main_camera.up , main_camera.view);  

        //glm_rotate(selected_model->model_mat,180,VEC3(0,1,0));

        render_to_texture(128,content_view_create_model_view);        
        memset(directory,0,sizeof(directory));
        sprintf(directory,"%s%s%s%i%s",pavon_the_game_project_folder,".thumbnails/",content_view->content_name,i,".jpg");
        texture_current_export_name = directory;
        texture_export(128);
        strcpy(content_view->thumbnail_image_path,directory);
    }

    main_camera = saved_camera;
    actual_model_array = prev_model_array;

    engine_memory_free_to_marker(memory_mark);
    models_loaded_for_create_thumbnails.data = NULL;
    
}

void editor_window_content_browser_load_thumbnails(){
    for (int i = 0; i < array_content_views.count; i++)
    {
        ContentView* content_view = array_get(&array_content_views,i);
        if(!content_view)
            continue;
        
        if(strcmp(content_view->thumbnail_image_path, "") == 0)
            return;
        PTexture new_texture;
        pe_load_texture(content_view->thumbnail_image_path,&new_texture);       

        content_view->draw.texture = new_texture.id;

    }
}

void editor_window_content_browser_new_content_view(const char* name,ContentType type  ,struct ContentViewPort* view_port){
    ContentView new_content_view;
    memset(&new_content_view,0,sizeof(ContentView));
    memcpy(new_content_view.content_name, name,strlen(name)-3);    

    new_content_view.text_size = 12;

    new_content_view.draw.shader = create_engine_shader(standart_vertex_shader,editor_standard_fragment_shader);
    new_content_view.draw.vertex = UI_plane_vertex_buffer_id;
    new_content_view.size[0] = 58;
    new_content_view.size[1] = 58;

    if(view_port->object_x_count < view_port->max_x){            
        if(view_port->object_x_count  != 0)
            view_port->last_x += 128;
        view_port->object_x_count ++;
    }else{
        view_port->last_y += 148;
        view_port->object_x_count  = 1;
        view_port->last_x = 64;
    }
    new_content_view.position[0] = view_port->last_x;
    new_content_view.position[1] = view_port->last_y;

    new_content_view.pixel_size = 64 + 12;    
		new_content_view.type = type;

    array_add(&array_content_views,&new_content_view);
}

void editor_window_content_get_models_path(){

  struct dirent *de; // Pointer for directory entry

  char directory[sizeof(project_manager_current_path) + 30];
  LOG("The project path: %s\n", project_manager_current_path);
  memset(directory, 0, sizeof(directory));
  strcat(directory, project_manager_current_path);
  strcat(directory, content_folder);

  DIR *dr = opendir(directory);

  if (dr == NULL) {
    LOG("Could not open current directory\n");
    LOG("Can't open: %s\n", directory);
    return;
  }

  int directory_count = 0;
  while ((de = readdir(dr)) != NULL) {
    directory_count++;
  }
  if (directory_count == 0) {
    text_render_in_screen_space(12, "No content in this project", 299, -300);
    closedir(dr);
    return;
  }
  rewinddir(dr);
  int model_count = 0;

  char model_names[directory_count][30];
  memset(model_names, 0, sizeof(model_names));
  for (int i = 0; i < directory_count; i++) {
    de = readdir(dr);
    int name_lenght = strlen(de->d_name);
    for (int n = 0; n < name_lenght; n++) {
      if (de->d_name[n] == '.') {
        if (strcmp(&de->d_name[n + 1], "pb") == 0) {
          strcpy(&model_names[model_count][0], de->d_name);
					
          model_count++;
        }
      }
    }
  }

  closedir(dr);

  array_clean(&array_finding_content);
  array_clean(&array_content_views);
  array_init(&array_finding_content, sizeof(ContentView *), (model_count));
  array_init(&array_content_views, sizeof(ContentView), (model_count));

  struct ContentViewPort new_view_port;
  memset(&new_view_port, 0, sizeof(struct ContentViewPort));
  new_view_port.last_x = 64;
  new_view_port.last_y = 64;
  new_view_port.max_x = camera_width_screen / 128;
  for (int i = 0; i < model_count; i++) {
		char full_file_path[sizeof(directory) + 100];
		memset(full_file_path,0,sizeof(full_file_path));
		strcat(full_file_path,directory);
		strcat(full_file_path,model_names[i]);
		ContentType type =	content_manager_get_content_type_from_binary(full_file_path);
		if(type == CONTENT_TYPE_NULL)
				continue;
    editor_window_content_browser_new_content_view(model_names[i],type,
                                                   &new_view_port);
  }
}

void editor_window_content_browser_close_window(){
	window_content_browser->focus = false;
	window_content_browser->initialized = false;
	editor_window_content_open = false;
	array_clean(&array_content_views);	
}

void editor_window_content_browser_update(){
}


void editor_window_content_init(){
  // glEnable(GL_DEPTH_TEST);
  // glEnable(GL_CULL_FACE);

  editor_window_content_get_models_path();
  // content_browser_window_create_contents_thumbnails();
  // editor_window_content_browser_load_thumbnails();

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  content_manager_init();
}

