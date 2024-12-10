#include "render_thread.h"
#include <engine/engine.h>
#include <engine/renderer/renderer.h>
#include <engine/threads.h>
#include <engine/types.h>

#include <editor/skeletal_editor.h>
#include <engine/text_renderer.h>

#include <GL/gl.h>

void engine_draw_elements(Array *elements) {
  for (size_t i = 0; i < elements->count; i++) {
    PModel *draw_model = array_get_pointer(elements, i);
    if (!draw_model)
      continue;
    draw_simgle_model(draw_model);
  }
  array_clean(elements);
}

void pe_render_skinned_elements(Array *elements) {
  for (size_t i = 0; i < elements->count; i++) {
    PSkinnedMeshComponent **skinp = array_get(elements, i);
    PSkinnedMeshComponent *skin = skinp[0];

    pe_render_skinned_model(skin);
  }

  array_clean(elements);
  array_clean(&frame_draw_skinned_elements);
}

void pe_frame_clean() {
  // clean frame
  array_clean(&models_for_test_occlusion);
  array_clean(&array_static_meshes_pointers);
  array_clean(&array_static_meshes_pointers_for_test_distance);
  array_clean(&array_skinned_mesh_pointers);
  array_clean(&array_skinned_mesh_for_distance_test);
  for_each_element_components(&clean_component_value);
  // end clean frame
}

void pe_render_thread_init() {
  
  if(pe_renderer_type == PEWMOPENGLES2){

    pe_shader_compile_std();

    glEnable(GL_MULTISAMPLE);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
  }



  if(pe_renderer_type == PEWMVULKAN){
    #if VULKAN
      pe_vk_init();
    #endif
  }

  if(pe_renderer_type == PEWMOPENGLES2){
    pe_gui_init();

    text_renderer_init();
  }

}

void pe_render_draw() {

  //************************


    pe_thread_control(&render_thread_commads);

    if (render_thread_definition.draw != NULL)
      render_thread_definition.draw();

  // end while
}

/*Start render thread and call pe_render_draw()*/
void pe_render_thread_start_and_draw() {
  thread_new_detached(pe_render_draw, NULL, "Render", &pe_th_render_id);
}

void pe_frame_static_fill(ComponentDefinition *definition) {
  if (definition->type == STATIC_MESH_COMPONENT) {

    StaticMeshComponent *mesh_comp = definition->data;
    if (mesh_comp->models_p.initialized == false)
      return;
    PModel *model = (PModel *)array_get_pointer(&mesh_comp->models_p, 0);
    // model->shader = mesh_comp->material.shader;
    model->material = mesh_comp->material;

    array_add_pointer(&frame_draw_static_elements, model);
  }
  if (definition->type == COMPONENT_SKINNED_MESH) {

    PSkinnedMeshComponent *skin_comp = definition->data;
    if (!skin_comp) {
      LOG("No skin component");
    }

    pe_render_skinned_model(skin_comp);
  }
}

// IMPORTAND
void pe_frame_draw() {

  glClearColor(pe_background_color[0], pe_background_color[1],
               pe_background_color[2], pe_background_color[3]);

  render_clear_buffer(RENDER_COLOR_BUFFER | RENDER_DEPTH_BUFFER);

  for_each_element_components(&update_per_frame_component);

  // test_elements_occlusion();
  // check_meshes_distance();
  for_each_element_components(&pe_frame_static_fill);

  for (int i = 0; i < actual_elements_array->count; i++) {
    Element *element = array_get(actual_elements_array, i);
  }

  engine_draw_elements(&frame_draw_static_elements);

  if(pe_renderer_type == PEWMVULKAN){
    pe_vk_draw_frame();
  }

  //draw_skeletal_bones();

  pe_frame_clean();
}
