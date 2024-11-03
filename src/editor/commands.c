#include "commands.h"
#include "editor.h"

#include "windows/content_browser.h"

#include <editor/project_manager.h>

#include "../engine/utils.h"
#include "../editor/editor_mode.h"
#include "HLOD_factory.h"
#include <stdbool.h>
#include <unistd.h>

int log_command_offset = 0;
bool activate_text_input_mode = false;

char first_char_command;
char* command_array_pointer;
char* argument_array_pointer;

int time_called = 0;
void pe_editor_parse_cmd_char(unsigned char character) {
  time_called++;
  if(time_called != 2)
    return;

  time_called=0;
  if (character == ':') {
    //LOG("## command mode\n");
    command_text_buffer[command_character_count] = character;
    command_character_count++;
    if (editor_sub_mode != EDITOR_SUB_MODE_TEXT_INPUT)
      change_to_editor_sub_mode(EDITOR_SUB_MODE_TEXT_INPUT);
    return;
  } else if (character == '/') {
    if (editor_sub_mode == EDITOR_SUB_MODE_NULL) {
      LOG("Search mode\n");
      command_text_buffer[command_character_count] = character;
      command_character_count++;
      editor_search_objects = true;
      change_to_editor_sub_mode(EDITOR_SUB_MODE_TEXT_INPUT);
      return;
    }
  }

  if (editor_sub_mode == EDITOR_SUB_MODE_TEXT_INPUT) {
    command_text_buffer[command_character_count] = character;
    command_character_count++;
  }
  if (editor_window_content_browser_hint && window_content_browser->focus) {
    command_text_buffer[command_character_count] = character;
    command_character_count++;
  }
}

void system_command(const char *command, const char *argument) {
  char final_command[strlen(command) + strlen(argument)];
  memset(final_command, 0, strlen(final_command));
  strcat(final_command, command);
  strcat(final_command, argument);

  if (fork() == 0) {
    system(final_command);
    exit(0);
  }
}

void command_parse_parameter(const char* command, const char*  parameter){
  if (strcmp(command, "project_new") == 0) {
    //		LOG("Parameter: %s\n",parameter);
//    project_manager_new(parameter);
  }
  if (strcmp(command, "project_open") == 0) {
    project_manager_open(parameter);
  }

  if (strcmp(command, "level_new") == 0) {
    editor_level_new(parameter);
  }
  if (strcmp(command, "level_save") == 0) {
    editor_level_save(parameter);
  }
}

void command_parse(const char *command) {
  int parameters = 0;
  int command_len = strlen(command);
  for (int i = 0; i < command_len; i++) {
    if (command[i] == ' ') {
      parameters++;
      char new_command_text[i];
      memcpy(new_command_text, command, i);
      new_command_text[i] = '\0';
      command_parse_parameter(new_command_text, &command[i + 1]);
    }
  }
}

void parse_command_with_len(const char* command, int len){
    
}


void parse_commmand_with_one_parameter(const char* command, float parameter){
    if(strcmp(command,"cluster") == 0){
        
        add_editor_native_element("HLOD Cluster");
        update_scale(VEC3(parameter,parameter,parameter));

        
        return;
    }

}

void command_while_not_space(const char* command){
    int command_len = strlen(command);
    for(int i = 0; i<command_len; i++){
        if(command[i] == ' '){
            char new_command_text[i];
            memcpy(new_command_text,command,i);
            LOG("%s\n",new_command_text);
            float parameter = atof(&command[i+1]);
            parse_commmand_with_one_parameter(new_command_text,parameter);
        }
    }
}

void text_input_mode(){
  if(key_released(&input.A))
    pe_editor_parse_cmd_char('a'); 
  if(key_released(&input.B))
    pe_editor_parse_cmd_char('b');
  if(key_released(&input.C))
    pe_editor_parse_cmd_char('c');
  if(key_released(&input.D))
    pe_editor_parse_cmd_char('d');
  if(key_released(&input.E))
    pe_editor_parse_cmd_char('e');
  if(key_released(&input.F))
    pe_editor_parse_cmd_char('f');
  if(key_released(&input.G))
    pe_editor_parse_cmd_char('g');
  if(key_released(&input.H))
    pe_editor_parse_cmd_char('h');
  if(key_released(&input.I))
    pe_editor_parse_cmd_char('i');
  if(key_released(&input.J))
    pe_editor_parse_cmd_char('j');
  if(key_released(&input.K))
    pe_editor_parse_cmd_char('k');
  if(key_released(&input.L))
    pe_editor_parse_cmd_char('l');
  if(key_released(&input.M))
    pe_editor_parse_cmd_char('m');
  if(key_released(&input.N))
    pe_editor_parse_cmd_char('n');
  if(key_released(&input.O))
    pe_editor_parse_cmd_char('o');
  if(key_released(&input.P))
    pe_editor_parse_cmd_char('p');
  if(key_released(&input.Q))
    pe_editor_parse_cmd_char('q');
  if(key_released(&input.R))
    pe_editor_parse_cmd_char('r');
  if(key_released(&input.S))
    pe_editor_parse_cmd_char('s');
  if(key_released(&input.T))
    pe_editor_parse_cmd_char('t');
  if(key_released(&input.U))
    pe_editor_parse_cmd_char('u');
  if(key_released(&input.X))
    pe_editor_parse_cmd_char('x');
  if(key_released(&input.Y))
    pe_editor_parse_cmd_char('y');
  if(key_released(&input.Z))
    pe_editor_parse_cmd_char('z');

    if(key_released(&input.ENTER)){
        parse_command(command_text_buffer);
        command_character_count = 0;
        save_commnad_history(command_text_buffer);
        memset(command_text_buffer,0,sizeof(command_text_buffer));
        
        change_to_editor_sub_mode(EDITOR_SUB_MODE_NULL);
        return;
    }
    if(key_released(&input.BACKSPACE)){
        command_character_count--;
        command_text_buffer[command_character_count] = '\0';
    }
    if(key_released(&input.KEY_UP)){
       
        FILE* file = fopen("/home/pavon/sources/PavonEngineC/Binaries/command_history.txt","r");
        if(!file)
            return;

        fseek(file, 0, SEEK_END);
     
        static const long max_len = 55 + 1;

        char buf[max_len + 1];

        /* now read that many bytes from the end of the file */
        fseek(file, -max_len, SEEK_END);
        int len = fread(buf,1, 55, file);

        buf[len] = '\0';

        /* and find the last newline character (there must be one, right?) */
        char *last_newline = strrchr(buf, '\n');
        char *last_line = last_newline+1;

        LOG("Last line: %s\n",last_line);
        memcpy(&command_text_buffer[command_character_count],last_line,strlen(last_line));

    }
}

int command_parse_from_command_line(int argc, char* argv[]){
	if(argc <= 1){
		return 0;	
	}	
	if(strcmp(argv[1] , "-o") == 0){
		memset(editor_level_open_path,0,sizeof(editor_level_open_path));
		strcpy(editor_level_open_path, argv[2]);			
		LOG("Open level: %s \n",editor_level_open_path);
		return 1;
	}
	
    
    
    pe_renderer_type = PEWMOPENGLES2;

    if(strcmp(argv[1] , "-vk") == 0){
       pe_renderer_type = PEWMVULKAN;

    }else if(strcmp(argv[1] , "-ogl") == 0){
        pe_renderer_type = PEWMOPENGLES2;
    }else{
        pe_renderer_type = PEWMOPENGLES2;
    }

         

	return 1;
}

void parse_command(const char *in_command) {

	char command[1000];
	memset(command,0,sizeof(command));
	strcpy(command,in_command);
	//LOG("Command lenght: %i\n",strlen(command));
//	LOG("In Command result: %s\n",command);
	first_char_command = command[1];

  //command_while_not_space(&command[1]);
  //command_parse(&command[1]);

  if (strcmp(&command[1], "gui") == 0) {
    change_to_editor_mode(EDITOR_MODE_GUI_EDITOR);
    return;
  }

  if (strcmp(&command[1], "compile") == 0) {
    LOG("compiling...\n");
    return;
  }

  if (strcmp(&command[1], "ac") == 0) {
    editor_generate_and_add_cube_element();
    return;
  }

  if (strcmp(&command[1], "hlod") == 0) {
    generate_HLODS(false);
    return;
  }
  if (strcmp(&command[1], "hlode") == 0) {
    generate_HLODS(true);
    gizmos_can_draw_spheres = false;
    return;
  }

  switch (first_char_command) {
  case 'w': {
    switch (editor_mode) {
    case EDITOR_MODE_GUI_EDITOR:
      save_gui_data(&command[3]);
      LOG("GUI saved: %s\n", &command[3]);
      return;
    case EDITOR_DEFAULT_MODE:
      save_level_data(&command[3]);
      LOG("Level saved: %s\n", &command[3]);
      return;
    default:
      break;
    }
  }
  case 'a': {
    editor_add_element_with_model_path(&command[3]);
    return;
  }
  case 't': {
    switch (editor_mode) {
    case EDITOR_MODE_GUI_EDITOR: {
      if (!selected_button)
        return;

      pe_gui_button_set_tex_with_path(selected_button, &command[3]);
      return;
    }

    case EDITOR_DEFAULT_MODE:
      add_texture_to_selected_element_with_image_path(&command[3]);
      return;

    default:
      break;
    }
    break;
  }
  case 'o': {
    switch (editor_mode) {
    case EDITOR_MODE_GUI_EDITOR:
      load_gui(&command[3]);
      LOG("GUI loaded\n");
      return;

    case EDITOR_DEFAULT_MODE:
      editor_level_open(&command[3]);
      LOG("Level loaded\n");
      return;

    default:
      break;
    }
  }
  case 'r': {
    reload_editor();
    LOG("reload\n");
    return;
  } break;
  case 'u': {
    isDrawUV = true;
    break;
  }
  case 'e': {
    data_export_select_element("/home/pavon/PavonTheGame/Content/export", true);
    // system("blender --python ../scripts/Blender/import.py");
    // reload_editor();
    // add_element_with_model_path("../assets/HLOD/out.gltf");
    return;
  }
  case 'q': {
    exit(0);
    return;
  }
  case 'g': {

    return;
  }
  case 's': {
    if (command[2] == 'z') {
      mat4 idenity;
      glm_mat4_identity(idenity);
      glm_mat4_copy(idenity, selected_element->transform->model_matrix);
      update_translation(VEC3(0, 0, 0));
      return;
    }

    if (command[3] == 'p') { // player
      player1 = selected_element;
      add_editor_native_element("Player Controller");
      return;
    }

    if (command[2] == 'n') {
      switch (editor_mode) {
      case EDITOR_MODE_GUI_EDITOR:
        strcpy(selected_button->name, &command[4]);
        return;

      case EDITOR_DEFAULT_MODE:
        strcpy(selected_element->name, &command[4]);
        return;

      default:
        break;
      }
    }
  }
  case 'd': {
    unsigned int duplicate_count = atoi(&command[3]);
    int duplicate_offset = atoi(&command[5]);
    Element *original = selected_element;
    for (int i = 0; i < duplicate_count; i++) {
      duplicate_selected_element(i, original);
      vec3 move = {duplicate_offset, 0, 0};
      update_translation(move);
    }
    LOG("duplicated %i\n", duplicate_count);
  }
  default:
    break;
  }
}
