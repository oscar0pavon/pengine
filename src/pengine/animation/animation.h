/*Created by pavon on 2019/07/17 */
#ifndef PE_ANIMATION_H
#define PE_ANIMATION_H

#define PATH_TYPE_NULL 0
#define PATH_TYPE_TRANSLATION 1
#define PATH_TYPE_ROTATION 2

#include "../array.h"
#include "../skeletal.h"

typedef struct PAnimationSampler {
  Array inputs;  // int
  Array outputs; // vec3 or vec4
} PAnimationSampler;

typedef struct PAnimationChannel {
  unsigned short int path_type;
  Node *node;
  PAnimationSampler sampler;
} PAnimationChannel;

typedef struct PAnimation {
  float time;
  bool loop;
  char name[48];
  Array channels; // PAnimationChannel
  float start;
  float end;
} PAnimation;

typedef struct PAnimationPlay {
  PAnimation *anim;
  PSkin *skin;
} PAnimationPlay;

Array array_animation_play_list;

void play_animation_by_name(PSkin *skin_component, const char *name,
                            bool loop);
void play_animation_list(float delta_seconds);
void pe_anim_nodes_update(PSkin *skin_component);

#ifdef DEBUG
bool update_vertex_bones_gizmos;
#endif

#endif // !PE_ANIMATION_H
