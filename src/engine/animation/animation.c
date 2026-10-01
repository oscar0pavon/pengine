#include "animation.h"

#include <cglm/mat4.h>
#include <engine/array.h>
#include <engine/engine.h>

void pe_anim_nodes_update(PSkin *skin_component) {
  if (!skin_component) {
    LOG("No skinned mesh component\n");
    return;
  }

  ZERO(skin_component->node_uniform);
  skin_component->node_uniform.joint_count = skin_component->joints.count;

  for (int i = 0; i < skin_component->joints.count; i++) {
    Node *joint = array_get(&skin_component->joints, i);

    mat4 local;
    get_global_matrix(joint, local);

    //TODO: use skin_component->transform->model_matrix instead of identity
    mat4 model_matrix;
    glm_mat4_identity(model_matrix);

    mat4 negative;
    glm_mat4_inv(model_matrix, negative);

    mat4 inverse_model;
    glm_mat4_inv(negative, inverse_model);

    mat4 inverse_dot_local;
    glm_mat4_mul(inverse_model, local, inverse_dot_local);

    mat4 *inverse_bind_matrix =
        array_get(&skin_component->inverse_bind_matrices, i);

    //INFO dereferenced: array_get() hands back a pointer to the mat4 element,
    //and glm_mat4_mul() takes the mat4 itself, which decays to vec4*
    mat4 joint_mat;
    glm_mat4_mul(inverse_dot_local, *inverse_bind_matrix, joint_mat);

    glm_mat4_copy(joint_mat, skin_component->node_uniform.joints_matrix[i]);
  }
}

void play_animation(PSkin *skin, PAnimation *animation, float delta_seconds) {
  animation->time += delta_seconds;
  float time = animation->time;

  for (int i = 0; i < animation->channels.count; i++) {
    PAnimationChannel *channel = array_get(&animation->channels, i);
    PAnimationSampler *sampler = &channel->sampler;

    Node *node = channel->node;
    if (!node) {
      LOG("No node in this channel");
      return;
    }

    for (int j = 0; j < sampler->inputs.count - 1; j++) {
      float *input0 = array_get(&sampler->inputs, j);
      float *input1 = array_get(&sampler->inputs, j + 1);
      if (time < *input0 || time > *input1)
        continue;

      float time_mix = (time - *input0) / (*input1 - *input0);
      switch (channel->path_type) {
      case PATH_TYPE_ROTATION: {
        float *quaternion0 = array_get(&sampler->outputs, j);
        float *quaternion1 = array_get(&sampler->outputs, j + 1);
        glm_quat_slerp(quaternion0, quaternion1, time_mix, node->rotation);
        break;
      }
      case PATH_TYPE_TRANSLATION: {
        float *position0 = array_get(&sampler->outputs, j);
        float *position1 = array_get(&sampler->outputs, j + 1);
        glm_vec3_lerp(position0, position1, time_mix, node->translation);
        break;
      }
      }
    }
  }

  pe_anim_nodes_update(skin);
}

void play_animation_by_name(PSkin *skin_component, const char *name,
                            bool loop) {
  if (skin_component->animations.count == 0) {
    LOGW("No animations in skinned mesh");
    return;
  }

  PAnimation *animation = NULL;
  for (int i = 0; i < skin_component->animations.count; i++) {
    PAnimation *candidate = array_get(&skin_component->animations, i);
    if (strcmp(name, candidate->name) == 0) {
      animation = candidate;
      break;
    }
  }

  if (animation == NULL) {
    LOG("Animation not found: %s\n", name);
    return;
  }

  for (int i = 0; i < array_animation_play_list.count; i++) {
    PAnimationPlay *play = array_get(&array_animation_play_list, i);
    //already the skin's current animation - leave its time alone, or every
    //caller re-asking for the same clip each frame would restart it from 0
    if (play->skin == skin_component && play->anim == animation)
      return;
  }

  //a skin plays one animation at a time: drop whatever else this skin had
  //queued before starting the new one, compacting the list in place
  int kept = 0;
  for (int i = 0; i < array_animation_play_list.count; i++) {
    PAnimationPlay *play = array_get(&array_animation_play_list, i);
    if (play->skin == skin_component)
      continue;
    if (kept != i)
      *(PAnimationPlay *)array_get(&array_animation_play_list, kept) = *play;
    kept++;
  }
  array_animation_play_list.count = kept;
  array_animation_play_list.bytes_size =
      kept * array_animation_play_list.element_bytes_size;

  animation->time = 0;
  animation->loop = loop;

  PAnimationPlay new_play;
  ZERO(new_play);
  new_play.anim = animation;
  new_play.skin = skin_component;
  array_add(&array_animation_play_list, &new_play);
}

void play_animation_list(float delta_seconds) {
  for (int i = 0; i < array_animation_play_list.count; i++) {
    PAnimationPlay *play = array_get(&array_animation_play_list, i);
    PAnimation *animation = play->anim;

    if (animation->time <= animation->end) {
      play_animation(play->skin, animation, delta_seconds);
#ifdef DEBUG
      update_vertex_bones_gizmos = true;
#endif
    } else if (animation->loop) {
      animation->time = 0;
    }
  }
}
