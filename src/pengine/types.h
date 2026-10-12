#ifndef PE_TYPES_H
#define PE_TYPES_H

#include <stdbool.h>

#include <cglm/types.h>
#include "array.h"
#include "numbers.h"

#ifdef ANDROID
struct android_app *app;
#endif

typedef struct PShaderStorageBufferObject {
  mat4 joints[35];
} PShaderStorageBufferObject;

typedef struct PEColorShader {
  float x;
  float y;
  float z;
} PEColorShader;

typedef struct PlayerStart {
  vec3 position;
  versor rotation;
} PlayerStart;

typedef void (*Action)(void);

typedef struct ActionPointer {
  int id;
  Action action;
} ActionPointer;

typedef struct {
  void (*command)(void *);
  bool executed;
  void *parameter;
} ExecuteCommand;

#endif // !PE_TYPES_H
