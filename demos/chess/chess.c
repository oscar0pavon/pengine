#include "chess.h"

#include <engine/files.h>
#include <engine/renderer/descriptor_set.h>
#include <engine/renderer/draw.h>
#include <engine/renderer/shaders.h>
#include <engine/renderer/uniform_buffer.h>
#include <engine/window_manager.h>

#define CHESS_SQUARES 64
#define CHESS_PIECES 32

//INFO these are plain scale factors. the ECS call these replaced took a delta
//and scaled by 1 + it, so the -0.5f and -0.8f the old code passed meant 0.5
//and 0.2 - reading them as factors makes every piece four times too big
#define CHESS_SQUARE_SCALE 0.5f
#define CHESS_PIECE_SCALE 0.2f

typedef enum ChessMeshId {
  CHESS_MESH_SQUARE,
  CHESS_MESH_PAWN,
  CHESS_MESH_ROOK,
  CHESS_MESH_BISHOP,
  CHESS_MESH_KNIGHT,
  CHESS_MESH_QUEEN,
  CHESS_MESH_KING,
  CHESS_MESH_COUNT
} ChessMeshId;

//loaded from the demo's own directory, so pchess has to be run from there
static const char *chess_mesh_paths[CHESS_MESH_COUNT] = {
    "cube.glb",   "pawn.glb",  "rook.glb", "bishop.glb",
    "knight.glb", "queen.glb", "king.glb"};

static vec4 square_color1 = {0, 0.2f, 0, 1};
static vec4 square_color2 = {1, 0.5f, 1, 1};

static vec4 piece_color1 = {0.5f, 1, 0.5f, 1};
static vec4 piece_color2 = {0, 0, 1, 1};

//one loaded copy of each mesh. every square and piece is instanced off these
static PModel chess_meshes[CHESS_MESH_COUNT];

static PShader chess_shader;

static ChessPiece chess_board[CHESS_SQUARES];
static ChessPiece chess_pieces[CHESS_PIECES];
static int chess_piece_count;

static ChessPiece *chess_knight_white;

static PCamera chess_camera_view_board;
static bool chess_saw_face;

static void chess_place(ChessPiece *piece, float x, float y, float z,
                        float scale, float angle) {

  pe_model_transform(&piece->model, VEC3(x, y, z), angle, VEC3(1, 0, 0),
                     VEC3(scale, scale, scale));
}

static void chess_move_piece(ChessPiece *piece, float x, float y) {
  chess_place(piece, x, y, 0, CHESS_PIECE_SCALE, 90);
}

static ChessPiece *chess_piece_new(ChessMeshId mesh, vec4 color, float x,
                                   float y) {

  if (chess_piece_count >= CHESS_PIECES) {
    LOG("Chess: no room left for another piece\n");
    return NULL;
  }

  ChessPiece *piece = &chess_pieces[chess_piece_count];
  chess_piece_count++;

  pe_vk_model_instance(&piece->model, &chess_meshes[mesh]);
  glm_vec4_copy(color, piece->color);

  chess_move_piece(piece, x, y);

  return piece;
}

static void chess_meshes_load() {

  PCreateShaderInfo shader_info;
  ZERO(shader_info);
  shader_info.out_shader = &chess_shader;
  shader_info.vertex_path = file_color_vert_spv;
  shader_info.fragment_path = file_color_frag_spv;
  //the layout built from pe_vk_descriptor_set_layout: one uniform buffer at
  //binding 0, which is all color_vert.vert reads
  shader_info.layout = pe_vk_pipeline_layout_with_descriptors;

  pe_vk_create_shader(&shader_info);

  for (int i = 0; i < CHESS_MESH_COUNT; i++) {
    pe_vk_load_model(&chess_meshes[i], chess_mesh_paths[i]);
    //the instances are memcpy'd from these, so the pipeline has to be in place
    //before the first pe_vk_model_instance() call
    chess_meshes[i].shader = chess_shader;
  }
}

static void chess_board_create() {

  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {

      ChessPiece *square = &chess_board[x * 8 + y];

      pe_vk_model_instance(&square->model, &chess_meshes[CHESS_MESH_SQUARE]);

      if ((x + y) % 2 == 0)
        glm_vec4_copy(square_color2, square->color);
      else
        glm_vec4_copy(square_color1, square->color);

      chess_place(square, x, y, -0.5f, CHESS_SQUARE_SCALE, 0);
    }
  }
}

static void chess_pieces_create() {

  for (int i = 0; i < 8; i++) {
    chess_piece_new(CHESS_MESH_PAWN, piece_color2, 1, i);
    chess_piece_new(CHESS_MESH_PAWN, piece_color1, 6, i);
  }

  chess_piece_new(CHESS_MESH_ROOK, piece_color2, 0, 0);
  chess_piece_new(CHESS_MESH_ROOK, piece_color2, 0, 7);
  chess_piece_new(CHESS_MESH_ROOK, piece_color1, 7, 0);
  chess_piece_new(CHESS_MESH_ROOK, piece_color1, 7, 7);

  chess_piece_new(CHESS_MESH_BISHOP, piece_color2, 0, 2);
  chess_piece_new(CHESS_MESH_BISHOP, piece_color2, 0, 5);
  chess_piece_new(CHESS_MESH_BISHOP, piece_color1, 7, 2);
  chess_piece_new(CHESS_MESH_BISHOP, piece_color1, 7, 5);

  chess_piece_new(CHESS_MESH_KNIGHT, piece_color2, 0, 1);
  chess_piece_new(CHESS_MESH_KNIGHT, piece_color2, 0, 6);
  chess_piece_new(CHESS_MESH_KNIGHT, piece_color1, 7, 6);
  chess_knight_white = chess_piece_new(CHESS_MESH_KNIGHT, piece_color1, 7, 1);

  chess_piece_new(CHESS_MESH_QUEEN, piece_color2, 0, 4);
  chess_piece_new(CHESS_MESH_QUEEN, piece_color1, 7, 4);

  chess_piece_new(CHESS_MESH_KING, piece_color2, 0, 3);
  chess_piece_new(CHESS_MESH_KING, piece_color1, 7, 3);
}

static void chess_camera_init() {

  camera_init(&main_camera);

  // x: back/forward z: up/down y: left/right
  init_vec3(-6, 3.5, 10, main_camera.position);

  camera_update(&main_camera);
  camera_rotate_control(-35, 0);

  camera_update(&main_camera);

  memcpy(&chess_camera_view_board, &main_camera, sizeof(PCamera));
}

static void chess_piece_draw(ChessPiece *piece, VkCommandBuffer *cmd_buffer,
                             uint32_t index) {

  PUniformBufferObject *ubo = &piece->model.uniform_buffer_object;

  glm_mat4_copy(piece->model.model_mat, ubo->model);
  glm_mat4_copy(main_camera.view, ubo->view);
  glm_mat4_copy(main_camera.projection, ubo->projection);
  glm_vec4_copy(piece->color, ubo->color);

  pe_vk_send_uniform_buffer(&piece->model, index);

  PDrawModelCommand draw;
  ZERO(draw);
  draw.model = &piece->model;
  draw.layout = pe_vk_pipeline_layout_with_descriptors;
  draw.command_buffer = *cmd_buffer;
  draw.image_index = index;

  pe_vk_draw_model(&draw);
}

//INFO the renderer calls this from inside the render pass. the engine has no
//scene of its own, so this walk is the whole of what chess draws
static void chess_draw_scene(PRenderTarget *target, VkCommandBuffer *cmd_buffer,
                             uint32_t index) {

  for (int i = 0; i < CHESS_SQUARES; i++)
    chess_piece_draw(&chess_board[i], cmd_buffer, index);

  for (int i = 0; i < chess_piece_count; i++)
    chess_piece_draw(&chess_pieces[i], cmd_buffer, index);
}

void chess_input() {

  if (key_released(&input.A))
    chess_move_piece(chess_knight_white, 4, 4);

  if (key_released(&input.W))
    chess_move_piece(chess_knight_white, 3, 5);

  if (key_released(&input.Y))
    chess_move_piece(chess_knight_white, 5, 0);

  if (key_released(&input.Q)) {
    LOG("Chess: exit pressed\n");
    exit(0);
  }

  if (key_released(&input.V)) {
    if (chess_saw_face) {
      memcpy(&main_camera, &chess_camera_view_board, sizeof(PCamera));
      camera_update(&main_camera);
      chess_saw_face = false;
      return;
    }

    camera_rotate_control(-10, 0);
    camera_update(&main_camera);
    chess_saw_face = true;
  }
}

void chess_init() {

  pe_change_background_color(0, 0.4f, 1, 1);

  chess_camera_init();

  chess_meshes_load();

  chess_board_create();

  chess_pieces_create();

  pe_vk_draw_scene = &chess_draw_scene;
}

void chess_loop() {}

void chess_draw() {}

int main() {
  PGame chess;
  ZERO(chess);
  chess.name = "Chess";
  chess.update = &chess_loop;
  chess.init = &chess_init;
  chess.draw = &chess_draw;
  chess.input = &chess_input;

  pe_renderer_type = PEWMVULKAN;

  //the renderer cannot know this: it decides whether pe_vk_create_surface()
  //builds a VkSurfaceKHR from the compositor's wl_surface or whether the
  //application drives KMS itself and has no surface at all
  is_wayland_window = true;

  pengine_run(&chess);

  return 0;
}
