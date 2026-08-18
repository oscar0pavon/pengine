#ifndef ENGINE_DATA_TYPES
#define ENGINE_DATA_TYPES

#include <stdbool.h>

#include <cglm/types.h>
#include "array.h"
#include "numbers.h"

#ifdef EDITOR
#include "../editor/types.h"
#endif
#ifdef ANDROID
struct android_app *app;
#endif

typedef struct TransformComponent TransformComponent;
typedef struct ComponentDefinition ComponentDefinition;

typedef struct PUniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 projection;
  vec4 light_position;
} PUniformBufferObject;

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

#endif // !ENGINE_DATA_TYPES
