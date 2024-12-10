#include "menus.h"
#include "editor.h"
#include <engine/components/components.h>
#include <engine/gui.h>
#include <engine/macros.h>
#include <string.h>

#define EDITOR_NATIVE_ELEMETN_COUNT 9
const char *elements_names[] = {"Empty Element", "Camera", "Player Start",
                                "Collider",      "Sphere", "Cube",
                                "Cyllinder",     "Floor",  "HLOD Cluster"};

const char *components_names[] = {"Camera Component", "Sphere Component",
                                  "Cube Component", "Transform Component",
                                  "SkinnedMesh"};

void text_menu_update(PETextMenu *menu) {

  if (menu->execute) {
    if (menu->draw_text_funtion != NULL)
      menu->draw_text_funtion(menu);
    else
      LOG("Menu draw function not assigned\n");

    if (menu->show) {
      if (key_released(&input.ESC)) {
        menu->execute = false;
        menu->show = false;
      }
      if (key_released(&input.J)) {
        if (menu->element_count > menu->actual_element_select)
          menu->actual_element_select++;
        else {
          menu->actual_element_select = 0;
        }
      }
      if (key_released(&input.K)) {
        if (menu->actual_element_select >= 1 && menu->element_count > 0)
          menu->actual_element_select--;
        else {
          menu->actual_element_select = menu->element_count;
        }
      }
      if (key_released(&input.ENTER)) {
        menu->element_selected = true;
        menu->show = false;
      }
    }
    if (menu->element_selected) {
      menu->execute = false;
      menu->show = false;
      menu->element_selected = false;

      if (menu->execute_function == NULL) {
        LOG("Menu execute function not assigned\n");
        return;
      }

      menu->execute_function(menu);
    }
  }
}

void menu_can_open_with_key(PETextMenu *menu, Key *open_key, int mods) {
  if (mods == -1) {
    if (input.SHIFT.pressed)
      return;
    if (key_released(open_key)) {
      menu->execute = true;
      menu->show = true;
    }
  } else {
    if (key_released(open_key)) {
      if (mods == open_key->mods) {
        menu->execute = true;
        menu->show = true;
      }
    }
  }
}

void menu_draw_menus() {
  for (int i = 0; i < menus.count; i++) {
    PETextMenu *menus_list = array_get(&menus, i);
    if (menus_list->editor_mode != editor_mode)
      continue;
    if (menus_list->menu_in_editor == false)
      continue;
    menu_can_open_with_key(menus_list, menus_list->open_key,
                           menus_list->mods_key);
    text_menu_update(menus_list);
  }
}

void pe_editor_menus_update() {

  if (editor_sub_mode != EDITOR_SUB_MODE_TEXT_INPUT) {
    menu_draw_menus();
  }
}

// All menus need to assing elements count
void menu_new_from_data(const char *name, PETextMenu *new_menu) {
  PETextMenu menu;
  memset(&menu, 0, sizeof(PETextMenu));
  menu.execute_function = new_menu->execute_function;
  menu.draw_text_funtion = new_menu->draw_text_funtion;
  menu.execute = false;
  menu.show = false;
  menu.menu_in_editor = true;
  menu.open_key = new_menu->open_key;
  menu.mods_key = new_menu->mods_key;
  menu.element_count = 0;
  menu.editor_mode = new_menu->editor_mode;
  strcpy(menu.name, name);
  array_add(&menus, &menu);
}

void menu_new(const char *name, Key *open_key, int mods_key,
              PETextMenuFunction draw_function,
              PETextMenuFunction execute_function) {
  PETextMenu menu;
  memset(&menu, 0, sizeof(PETextMenu));
  menu.execute_function = execute_function;
  menu.draw_text_funtion = draw_function;
  menu.execute = false;
  menu.menu_in_editor = true;
  menu.show = false;
  menu.open_key = open_key;
  menu.mods_key = mods_key;
  menu.element_count = 0;
  strcpy(menu.name, name);
  array_add(&menus, &menu);
}

void draw_available_components(PETextMenu *menu) {
  float text_size = 12;
  set_text_size(text_size);
  menu->text_size = text_size;
  menu->element_count = 5;
  for (int i = 0; i < 4; i++) {

    const char *name = components_names[i];

    draw_element_text_list(menu, name, i);
  }
}

void draw_animations_names(PETextMenu *menu) {
  PSkinnedMeshComponent *skin_component = pe_comp_get(COMPONENT_SKINNED_MESH);
  if (!skin_component) {
    return;
  }

  float text_size = 12;
  set_text_size(text_size);
  menu->text_size = text_size;
  for (int i = 0; i < skin_component->animations.count; i++) {
    Animation *animation = array_get(&skin_component->animations, i);
    char *name = animation->name;

    draw_element_text_list(menu, name, i);

    strcpy(menu->text_for_action, name);
  }
}

void menu_action_play_animation(PETextMenu *menu) {
  PSkinnedMeshComponent *skin_component = pe_comp_get(COMPONENT_SKINNED_MESH);

  play_animation_by_name(skin_component, menu->text_for_action, true);
}

void menu_action_select_component_from_selected_element(PETextMenu *menu) {
  current_component_selected = array_get(&selected_element->components, 2);
  if (current_component_selected)
    LOG("Component selected\n");
}

void draw_components_from_selected_element(PETextMenu *menu) {
  float text_size = 12;
  set_text_size(text_size);
  menu->text_size = text_size;
  if (selected_element == NULL)
    return;
  if (selected_element->components.count == 0) {
    draw_element_text_list(menu, "No Components", 0);
  }

  for (int i = 0; i < selected_element->components.count; i++) {
    char *name;
    ComponentDefinition *component =
        array_get(&selected_element->components, i);
    switch (component->type) {
    case TRASNFORM_COMPONENT:
      name = "Transfrom Component";
      break;
    case CAMERA_COMPONENT:
      name = "Camera Component";
      break;

    case STATIC_MESH_COMPONENT:
      name = "Static Mesh Component";
      break;
    case LEVEL_OF_DETAIL_COMPONENT:
      name = "Level of Detail Component";
      break;
    case COMPONENT_SKINNED_MESH:
      name = "Skinned Mesh";
      break;
    default:
      break;
    }

    draw_element_text_list(menu, name, i);
  }
}

void menu_action_add_component_to_select_element(PETextMenu *menu) {
  if (strcmp("Transform Component", menu->text_for_action) == 0) {
    TransformComponent transform;
    init_transfrom_component(&transform);
    add_component_to_selected_element(sizeof(TransformComponent), &transform,
                                      TRASNFORM_COMPONENT);

  } else if (strcmp("Camera Component", menu->text_for_action) == 0) {
    add_camera_component_to_selected_element();
  }
}

void menu_action_add_element(PETextMenu *menu) {
  int name_lenght = strlen(menu->text_for_action);
  for (int n = 0; n < name_lenght; n++) {
    if (menu->text_for_action[n] == '.') {

      if (strcmp(&menu->text_for_action[n + 1], "blend") == 0) {
        char open_blender_file_command[] = "blender --background ";
        char python_command[] = " --python ../scripts/Blender/export.py";
        char relative_path[] = "../assets/";
        char python_argument[] = " -- ";
        char file_name[] = "blender_test.gltf";
        char final_command[strlen(open_blender_file_command) +
                           strlen(relative_path) +
                           strlen(menu->text_for_action) +
                           strlen(python_argument) + strlen(python_command) +
                           strlen(file_name)];
        memset(final_command, 0, strlen(final_command));

        strcat(final_command, open_blender_file_command);
        strcat(final_command, relative_path);
        strcat(final_command, menu->text_for_action);
        strcat(final_command, python_command);
        strcat(final_command, python_argument);
        strcat(final_command, file_name);

        system(final_command); // open and export selected objects

        char new_glTF_relative_path[] = "../generated_glTF_files/";
        char new_file_path[strlen(new_glTF_relative_path) + strlen(file_name)];
        memset(new_file_path, 0, strlen(new_file_path));
        strcat(new_file_path, new_glTF_relative_path);
        strcat(new_file_path, file_name);
        add_element_with_model_path(new_file_path);

        selected_element->editor_data.has_blend_file = true;
        strcpy(selected_element->editor_data.blend_file_path,
               menu->text_for_action);
        return;
      }
    }
  }
  // if not blend file
  add_element_with_model_path(menu->text_for_action);
  update_translation(main_camera.position);
  vec3 new_position;
  glm_vec3_scale(main_camera.front, 4, new_position);
  update_translation(new_position);
}

void menu_action_add_texture_to_element(PETextMenu *menu) {
  add_texture_to_selected_element_with_image_path(menu->text_for_action);
}

void menu_action_select_element(PETextMenu *menu) {
  int id = menu->actual_element_select;
  Element *element = NULL;

  if (!input.SHIFT.pressed) {
    if (selected_element != NULL)
      selected_element->selected = false;
  } else {
    array_add(&selected_elements_id, &selected_element->id);
  }

  element = array_get(&editor_elements, id);
  if (element == NULL)
    return;
  element->selected = true;
  selected_element = element;
  if (input.SHIFT.pressed) {
    array_add(&selected_elements_id, &selected_element->id);
  }

  for (int i = 0; i < selected_elements_id.count; i++) {
    unsigned short int *id = array_get(&selected_elements_id, i);
    unsigned short int id_number;
    memcpy(&id_number, id, sizeof(unsigned short int));
    LOG("seleteted: %i\n", id_number);
  }
  LOG("Selected element\n");
}

void menu_action_add_editor_native_element(PETextMenu *menu) {

  add_editor_native_element(menu->text_for_action);
  LOG("Add editor native element: %s\n", menu->text_for_action);
}

void menu_action_draw_native_editor_elments(PETextMenu *menu) {
  set_text_size(12);
  menu->text_size = 12;
  menu->element_count = 9;

  for (int i = 0; i < EDITOR_NATIVE_ELEMETN_COUNT; i++) {

    const char *name = elements_names[i];
    draw_element_text_list(menu, name, i);
  }
  strcpy(menu->text_for_action, elements_names[menu->actual_element_select]);
}

void menu_action_draw_editor_elements(PETextMenu *menu) {
  float text_size = 12;
  set_text_size(text_size);
  menu->text_size = text_size;
  if (editor_elements.count == 0) {
    draw_element_text_list(menu, "No Elements", 0);
  }
  menu->element_count = 0;
  for (int i = 0; i < editor_elements.count; i++) {
    Element *element = (Element *)array_get(&editor_elements, i);
    char *name = element->name;
    if (strcmp(element->name, "") == 0) {
      if (strcmp(element->name, "") == 0)
        name = "no_name";
      else
        name = element->name;
    }
    menu->element_count++;
    draw_element_text_list(menu, name, i);
  }
}

void menu_action_select_gui_element(PETextMenu *menu) {
  selected_button =
      array_get(actual_buttons_array, menu->actual_element_select);
}

void menu_action_draw_gui_elements(PETextMenu *menu) {
  menu->text_size = 12;
  menu->element_count = actual_buttons_array->count;
  for (int i = 0; i < actual_buttons_array->count; i++) {
    Button *button = array_get(actual_buttons_array, i);
    draw_element_text_list(menu, button->name, i);
  }
}

void pe_menu_loaded_tex_draw(PETextMenu *menu) {
  menu->text_size = 12;
  menu->element_count = pe_arr_tex_paths.count;
  FOR(pe_arr_tex_paths.count) {
    char *path = array_get(&pe_arr_tex_paths, i);
    draw_element_text_list(menu, path, i);
    menu->actual_element_select = i;
  }
}

void pe_menu_loaded_tex_exec(PETextMenu *menu) {
  pe_tex_loaded_to_model(menu->actual_element_select);
}

void menus_init() {
  ZERO(add_element_menu);
  ZERO(menu_add_texture);
  ZERO(menu_editor_element_list);

  mark_id = 0;

  /*Text Menu functions */
  add_element_menu.execute_function = &menu_action_add_element;
  add_element_menu.type = MENU_TYPE_ADD_MODEL;

  menu_add_texture.type = MENU_TYPE_ADD_TEXTURE;
  menu_add_texture.execute_function = &menu_action_add_texture_to_element;

  // New way to create texts menus
  array_init(&menus, sizeof(PETextMenu), 10);

  menu_new("Element Component List", &input.P, -1,
           &draw_components_from_selected_element,
           &menu_action_select_component_from_selected_element);

  menu_new("Add Component", &input.C, -1, &draw_available_components,
           &menu_action_add_component_to_select_element);

  PETextMenu native_elemets;
  native_elemets.draw_text_funtion = &menu_action_draw_native_editor_elments;
  native_elemets.execute_function = &menu_action_add_editor_native_element;
  native_elemets.open_key = &input.I;
  
  //native_elemets.mods_key = GLFW_MOD_SHIFT;
 
  native_elemets.editor_mode = EDITOR_DEFAULT_MODE;
  menu_new_from_data("NativeElements", &native_elemets);

  PETextMenu editor_elements;
  editor_elements.execute_function = &menu_action_select_element;
  editor_elements.draw_text_funtion = &menu_action_draw_editor_elements;
  editor_elements.open_key = &input.L;
  editor_elements.mods_key = -1;
  editor_elements.editor_mode = EDITOR_DEFAULT_MODE;
  menu_new_from_data("Elements", &editor_elements);

  PETextMenu animation_menu;
  animation_menu.open_key = &input.B;
  animation_menu.mods_key = -1;
  animation_menu.draw_text_funtion = &draw_animations_names;
  animation_menu.execute_function = &menu_action_play_animation;
  animation_menu.editor_mode = EDITOR_DEFAULT_MODE;
  menu_new_from_data("Animations", &animation_menu);

  PETextMenu gui_elements;
  ZERO(gui_elements);
  gui_elements.open_key = &input.Q;
  gui_elements.mods_key = -1;
  gui_elements.draw_text_funtion = &menu_action_draw_gui_elements;
  gui_elements.execute_function = &menu_action_select_gui_element;
  gui_elements.editor_mode = EDITOR_MODE_GUI_EDITOR;
  menu_new_from_data("GUIElements", &gui_elements);

  PETextMenu loaded_textures;
  ZERO(loaded_textures);
  loaded_textures.open_key = &input.T;

  //loaded_textures.mods_key = GLFW_MOD_SHIFT;

  loaded_textures.editor_mode = EDITOR_DEFAULT_MODE;
  loaded_textures.draw_text_funtion = &pe_menu_loaded_tex_draw;
  loaded_textures.execute_function = &pe_menu_loaded_tex_exec;
  menu_new_from_data("LoadedTex", &loaded_textures);
}
