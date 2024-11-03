#include "chess.h"
#include "ThirdParty/cglm/mat4.h"
#include "engine/elements.h"
#include <editor/skeletal_editor.h>
#include <engine/animation/node.h>
#include <engine/shader.h>

#include <engine/files.h>

vec4 color1 = {0, 0.2, 0, 1};
vec4 color2 = {1, 0.5, 1, 1};

vec4 piece_color1 = {0.5, 1, 0.5, 1};
vec4 piece_color2 = {0, 0, 1, 1};

PMesh check_mesh;
PMaterial check_board_mat2;
PMaterial check_board_mat1;

PMaterial piece_mat2;
PMaterial piece_mat1;

PSkinnedMeshComponent *human_skin_component;

Element *knight_white;

CameraComponent chess_camera_view_board;

bool chess_saw_face;
PMesh chess_get_mesh() {
  StaticMeshComponent *mesh =
      get_component_from_element(selected_element, STATIC_MESH_COMPONENT);

  PModel *original_check_mesh = array_get_pointer(&mesh->models_p, 0);
  return original_check_mesh->mesh;
}

void chess_create_notation() {}

void chess_piece_set_pos(vec2 pos) {

  pe_element_set_position(selected_element, VEC3(pos[0], pos[1], 0));
}

void chess_piece_init_scale() {
  float scale = -0.8f;
  pe_element_set_scale(VEC3(scale, scale, scale));
}

void chess_move_piece(vec2 pos) { chess_piece_set_pos(pos); }

void chess_piece_reset_scale_rot() {

  pe_element_rotate(selected_element, -90, VEC3(1, 0, 0));
  float scale = 0.8f;
  pe_element_set_scale(VEC3(scale, scale, scale));
  chess_move_piece(VEC2(0, 0));
}

void chess_piece_init_scale_rot() {

  chess_piece_init_scale();
  pe_element_rotate(selected_element, 90, VEC3(1, 0, 0));
}

void chess_piece_movement(int x, int y) {
  //  chess_piece_reset_scale_rot();
  //  chess_move_piece(VEC2(x,y));
  // chess_piece_init_scale_rot();

  TransformComponent *transform = pe_comp_get(TRASNFORM_COMPONENT);
  if (!transform)
    return;

  glm_mat4_identity(transform->model_matrix);
  chess_move_piece(VEC2(x, y));
  chess_piece_init_scale_rot();

  LOG("########## chess movement %i %i", x, y);
}

void chess_board_create() {

  // add_element_with_model_path("/home/pavon/PavonEngine/NativeContent/Editor/cube.glb");
  add_element_with_model_path("/sdcard/Download/NativeContent/Editor/cube.glb");

  StaticMeshComponent *mesh =
      get_component_from_element(selected_element, STATIC_MESH_COMPONENT);
  mesh->material = check_board_mat1;

  ZERO(check_mesh);

  float scale_board = -0.5f;
  pe_element_set_scale(VEC3(scale_board, scale_board, scale_board));

  PModel *original_check_mesh = array_get_pointer(&mesh->models_p, 0);
  check_mesh = original_check_mesh->mesh;

  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      new_empty_element();

      add_transform_component_to_selected_element();

      pe_comp_static_mesh_add_to_element();

      pe_element_comp_init();

      StaticMeshComponent *mesh2 =
          get_component_from_element(selected_element, STATIC_MESH_COMPONENT);

      if ((x + y) % 2 == 0) {
        mesh2->material = check_board_mat2;

      } else {

        mesh2->material = check_board_mat1;
      }

      selected_model->mesh = check_mesh;

      pe_element_set_position(selected_element, VEC3(x, y, -0.5));
      float scale = -0.5f;
      pe_element_set_scale(VEC3(scale, scale, scale));
    }
  }
}
void chess_input() {
  if (key_released(&input.A)) {
    selected_element = knight_white;
    chess_piece_movement(4, 4);
  }

  if (key_released(&input.W)) {
    selected_element = knight_white;
    chess_piece_movement(3, 5);
  }
  if (key_released(&input.Y)) {
    selected_element = knight_white;
    chess_piece_movement(5, 0);
  }

  if (key_released(&input.Q)) {
    LOG("###### Exit pressed");
    exit(0);
  }

  if (key_released(&input.V)) {
    if (chess_saw_face == true) {

      memcpy(&main_camera, &chess_camera_view_board, sizeof(CameraComponent));
      camera_update(&main_camera);
      chess_saw_face = false;
      return;
    }
    if (chess_saw_face == false) {
      camera_rotate_control(-10, 0);
      camera_update(&main_camera);
      chess_saw_face = true;
    }
  }

  //  LOG("###### Touch %f %f",touch_position_x,touch_position_y);
}
void chess_init_materials() {

  ZERO(check_board_mat1);
  check_board_mat1.shader = shader_standard_color;
  glm_vec4_copy(color1, check_board_mat1.color);

  ZERO(check_board_mat2);
  check_board_mat2.shader = shader_standard_color;
  glm_vec4_copy(color2, check_board_mat2.color);

  ZERO(piece_mat1);
  piece_mat1.shader = shader_standard_color;
  glm_vec4_copy(piece_color1, piece_mat1.color);

  ZERO(piece_mat2);
  piece_mat2.shader = shader_standard_color;
  glm_vec4_copy(piece_color2, piece_mat2.color);
}
void chess_new_empty() {

  new_empty_element();

  add_transform_component_to_selected_element();

  pe_comp_static_mesh_add_to_element();

  pe_element_comp_init();
}
void chess_piece_set_mesh(PMesh mesh) {

  StaticMeshComponent *mesh_comp =
      get_component_from_element(selected_element, STATIC_MESH_COMPONENT);

  selected_model->mesh = mesh;
}
void chess_create_leaders() {

  add_element_with_model_path(file_reina_glb);
  chess_piece_set_pos(VEC2(7, 4));
  chess_piece_init_scale();
  pe_element_rotate(selected_element, 90, VEC3(1, 0, 0));

  pe_element_set_material(piece_mat1);

  PMesh queen = chess_get_mesh();

  chess_new_empty();
  chess_piece_set_mesh(queen);
  pe_element_set_material(piece_mat2);
  chess_move_piece(VEC2(0, 4));
  chess_piece_init_scale();
  pe_element_rotate(selected_element, 90, VEC3(1, 0, 0));

  add_element_with_model_path("/sdcard/Download/chess/rey.glb");
  PMesh king = chess_get_mesh();

  chess_move_piece(VEC2(7, 3));
  chess_piece_init_scale();
  pe_element_rotate(selected_element, 90, VEC3(1, 0, 0));
  pe_element_set_material(piece_mat1);

  chess_new_empty();
  chess_piece_set_mesh(king);
  pe_element_set_material(piece_mat2);
  chess_move_piece(VEC2(0, 3));
  chess_piece_init_scale();
  pe_element_rotate(selected_element, 90, VEC3(1, 0, 0));
}

void chess_create_knight() {

  add_element_with_model_path("/sdcard/Download/chess/caballo.glb");

  chess_piece_set_pos(VEC2(0, 1));
  chess_piece_init_scale();
  pe_element_rotate(selected_element, 90, VEC3(1, 0, 0));

  PMesh knight = chess_get_mesh();

  pe_element_set_material(piece_mat2);

  chess_new_empty();
  chess_piece_set_mesh(knight);
  pe_element_set_material(piece_mat2);
  chess_move_piece(VEC2(0, 6));
  chess_piece_init_scale();
  pe_element_rotate(selected_element, 90, VEC3(1, 0, 0));

  chess_new_empty();
  chess_piece_set_mesh(knight);
  pe_element_set_material(piece_mat1);
  chess_move_piece(VEC2(7, 6));
  chess_piece_init_scale();
  pe_element_rotate(selected_element, 90, VEC3(1, 0, 0));

  chess_new_empty();
  chess_piece_set_mesh(knight);
  pe_element_set_material(piece_mat1);
  chess_move_piece(VEC2(7, 1));
  chess_piece_init_scale();
  pe_element_rotate(selected_element, 90, VEC3(1, 0, 0));
  knight_white = selected_element;
}

void chess_create_bishop() {

  add_element_with_model_path("/sdcard/Download/chess/alfil.glb");

  chess_piece_set_pos(VEC2(0, 2));
  chess_piece_init_scale();
  pe_element_rotate(selected_element, 90, VEC3(1, 0, 0));

  pe_element_set_material(piece_mat2);

  PMesh bishop = chess_get_mesh();

  chess_new_empty();

  chess_piece_set_mesh(bishop);

  pe_element_set_material(piece_mat2);

  chess_move_piece(VEC2(0, 5));
  chess_piece_init_scale();
  pe_element_rotate(selected_element, 90, VEC3(1, 0, 0));

  chess_new_empty();

  chess_piece_set_mesh(bishop);

  pe_element_set_material(piece_mat1);

  chess_move_piece(VEC2(7, 5));
  chess_piece_init_scale();
  pe_element_rotate(selected_element, 90, VEC3(1, 0, 0));

  chess_new_empty();

  chess_piece_set_mesh(bishop);

  pe_element_set_material(piece_mat1);

  chess_move_piece(VEC2(7, 2));
  chess_piece_init_scale();
  pe_element_rotate(selected_element, 90, VEC3(1, 0, 0));
}

void chess_create_pawn() {

  add_element_with_model_path(file_peon_glb);

  chess_piece_set_pos(VEC2(1, 0));
  chess_piece_init_scale();
  pe_element_set_material(piece_mat2);
  StaticMeshComponent *pawn_mesh_comp =
      get_component_from_element(selected_element, STATIC_MESH_COMPONENT);

  PModel *model = array_get_pointer(&pawn_mesh_comp->models_p, 0);
  PMesh pawn_mesh = model->mesh;

  for (int i = 1; i < 8; i++) {
    chess_new_empty();
    chess_piece_set_mesh(pawn_mesh);

    chess_move_piece(VEC2(1, i));
    chess_piece_init_scale();
    pe_element_rotate(selected_element, 90, VEC3(1, 0, 0));
    pe_element_set_material(piece_mat2);
  }

  for (int i = 0; i < 8; i++) {
    chess_new_empty();

    chess_piece_set_mesh(pawn_mesh);

    chess_move_piece(VEC2(6, i));
    chess_piece_init_scale();
    pe_element_rotate(selected_element, 90, VEC3(1, 0, 0));
    pe_element_set_material(piece_mat1);
  }
}
void chess_piece_new(float x, float y, PMaterial material, PMesh mesh) {

  chess_new_empty();
  chess_piece_set_mesh(mesh);

  pe_element_set_material(material);

  chess_piece_set_pos(VEC2(x, y));
  chess_piece_init_scale();
  pe_element_rotate(selected_element, 90, VEC3(1, 0, 0));
}
void chess_create_rooks() {

  add_element_with_model_path("/sdcard/Download/chess/torre.glb");
  chess_piece_set_pos(VEC2(7, 7));
  chess_piece_init_scale();
  pe_element_rotate(selected_element, 90, VEC3(1, 0, 0));

  pe_element_set_material(piece_mat1);

  PMesh rook_mesh = chess_get_mesh();

  chess_new_empty();
  chess_piece_set_mesh(rook_mesh);

  pe_element_set_material(piece_mat1);

  chess_piece_set_pos(VEC2(7, 0));
  chess_piece_init_scale();
  pe_element_rotate(selected_element, 90, VEC3(1, 0, 0));

  chess_piece_new(0, 0, piece_mat2, rook_mesh);
  chess_piece_new(0, 7, piece_mat2, rook_mesh);
}

void chess_pieces_create() {

  chess_create_pawn();
  chess_create_rooks();
  chess_create_bishop();
  chess_create_leaders();
  chess_create_knight();
}

void chess_camera_init() {

  camera_init(&main_camera);
  init_vec3(-6, 3.5, 10,
            main_camera.position); // x: back/forward z: up/down y: left/right
  // init_vec3(-6,3.5,8, main_camera.position);

  // init_vec3(-6,0,8, main_camera.position);//test human

  camera_update(&main_camera);
  camera_rotate_control(-35, 0);

  camera_update(&main_camera);

  memcpy(&chess_camera_view_board, &main_camera, sizeof(CameraComponent));
}

void chess_human_create() {
  LOG("###########HUman created and selected_element ");

  // add_element_with_model_path("/sdcard/Download/chess_human2.glb");
  add_element_with_model_path(
      "/home/pavon/PavonEngine/NativeContent/Editor/simple_skeletal.glb");

  // pe_element_set_position(selected_element,VEC3(10,4,-10));
  // pe_element_rotate(selected_element, -90, VEC3(0,0,1));

  //  float scale = -0.2f;
  // pe_element_set_scale(VEC3(scale,scale,scale)) ;

  PSkinnedMeshComponent *human_comp =
      get_component_from_element(selected_element, COMPONENT_SKINNED_MESH);
  if (!human_comp) {
    LOG("********Human component not found");
    return;
  }
  human_comp->mesh->material = piece_mat1;
  human_skin_component = human_comp;

  for (int i = 0; i < human_skin_component->mesh->vertex_array.count; i++) {
    PVertex *v = array_get(&human_skin_component->mesh->vertex_array, i);
    // LOG("############ UV: %f %f",v->uv[0],v->uv[1]);
  }

  // add_texture_to_selected_element_with_image_path(
  //    "/sdcard/Download/ImageConverter/Muro_head_dm.png");

  // add_texture_to_selected_element_with_image_path(
  //    "/sdcard/Download/ImageConverter/Muro_body_dm.png");

  // pe_skeletal_editor_init_for(human_skin_component);
}

void chess_init() {

  pe_change_background_color(0, 0.4f, 1, 1);

  chess_camera_init();

  chess_init_materials();

  chess_board_create();

  chess_pieces_create();

  // chess_human_create();
}

void chess_loop() {}

void chess_draw() {

  if (key_released(&input.R)) {

    if (!human_skin_component) {
      LOG("######### human skin component is NULL");
      return;
    }
    if (human_skin_component->joints.count == 0) {
      LOG("######### human skin component joints ZERO");
    }

    Node *node1 = pe_node_by_name(&human_skin_component->joints, "Bone.007");

    if (!node1) {
      LOG("######## NOde not found");
      return;
    }

    TransformComponent *knigt_trasform =
        get_component_from_element(knight_white, TRASNFORM_COMPONENT);
    if (!knigt_trasform) {
      LOG("########### No knight transform component");
    }

    StaticMeshComponent *knight_mesh_comp =
        get_component_from_element(knight_white, STATIC_MESH_COMPONENT);
    if (!knight_mesh_comp) {
      LOG("########### No mesh compon knight witeh");
    }

    PModel *knight_model = array_get(&knight_mesh_comp->models_p, 0);
    if (!knight_model) {
      LOG("#### Not model getted from knight model");
    }

    pe_skeletal_update_draw_vertices(human_skin_component);
    pe_anim_nodes_update(human_skin_component);
  }
}

PGame *chess_main(PGame *chess) {

  chess->name = "Chess";
  chess->loop = &chess_loop;
  chess->init = &chess_init;
  chess->draw = &chess_draw;
  chess->input = &chess_input;
  game = chess; // need for egl context creation

  return chess;
}
