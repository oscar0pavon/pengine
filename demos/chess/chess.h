#ifndef CHESS
#define CHESS

#include <engine/engine.h>
#include <engine/game.h>

//INFO a piece owns a whole PModel. the vertex and index buffers inside it are
//shared with the mesh the piece was instanced from, but the uniform buffers
//and descriptor sets are the piece's own, and those are what carry its
//transform and colour to the shader
typedef struct ChessPiece{
  PModel model;
  vec4 color;
}ChessPiece;

void chess_init();
void chess_loop();
void chess_draw();
void chess_input();

#endif
