#include "animation.h"
#include <engine/engine.h>
void pe_anim_nodes_update(SkinnedMeshComponent *skin_component) {
  if (!skin_component) {
    LOG("No skinned mesh component\n");
    return;
  }

  for (int i = 0; i < skin_component->joints.count; i++) {
    Node *joint = (Node *)array_get(&skin_component->joints, i);

    mat4 local;
    get_global_matrix(joint, local);
    // needed for transform
    mat4 negative;
    glm_mat4_inv(skin_component->transform->model_matrix, negative);
    //
    mat4 inverse_model;
    mat4 inverse_dot_local;
    mat4 joint_mat;

    glm_mat4_inv(negative, inverse_model);
    glm_mat4_mul(inverse_model, local, inverse_dot_local);
    glm_mat4_mul(inverse_dot_local, skin_component->inverse_bind_matrices[i],
                 joint_mat);

    glm_mat4_copy(joint_mat, skin_component->node_uniform.joints_matrix[i]);
  }
}

void play_animation(SkinnedMeshComponent *skin, Animation *animation) {
  animation->time += 0.01;
  float time = animation->time;
  for (int i = 0; i < animation->channels.count; i++) {
    AnimationChannel *channel = array_get(&animation->channels, i);
    AnimationSampler *sampler = &channel->sampler;

    Node *node = channel->node;
    if (!node) {
      LOGW("No node in this channel");
      return;
    }

    for (int j = 0; j < sampler->inputs.count - 1; j++) {
      float *input0 = array_get(&sampler->inputs, j);
      float *input1 = array_get(&sampler->inputs, j + 1);

      if ((time >= *input0) && (time <= *input1)) {
        float time_mix = (time - *input0) / (*input1 - *input0);
        switch (channel->path_type) {
        case PATH_TYPE_ROTATION: {
          float *quaternion0 = array_get(&sampler->outputs, j);
          float *quaternion1 = array_get(&sampler->outputs, j + 1);

          vec4 interpolated;
          glm_vec4_lerp(quaternion0, quaternion1, time_mix, interpolated);
          glm_vec4_normalize_to(interpolated, node->rotation);
          break;
        }
        case PATH_TYPE_TRANSLATION: {
          float *position1 = array_get(&sampler->outputs, j);
          float *position2 = array_get(&sampler->outputs, j + 1);
          glm_vec3_lerp(position1, position2, time_mix, node->translation);
          break;
        }
        }
      } // end if time >=
    }   // end for inputs
  }

  pe_anim_nodes_update(skin);
}

void play_animation_by_name(SkinnedMeshComponent *skin_component,
                            const char *name, bool loop) {
  Animation *animation = NULL;
  if (skin_component->animations.count == 0) {
    LOGW("No animations in skinned mesh");
    return;
  }
  for (int i = 0; i < skin_component->animations.count; i++) {
    Animation *geted_animation = array_get(&skin_component->animations, i);
    LOG("Comparing geted animation %s , with %s \n", geted_animation->name,
        name);
    if (strcmp(name, geted_animation->name) == 0) {
      animation = geted_animation;
      break;
    }
  }

  if (animation == NULL) {
    LOG("Animation not found: %s\n", name);
    return;
  }
  animation->loop = loop;
  PEAnimationPlay new_play;
  ZERO(new_play);
  new_play.anim = animation;
  new_play.skin = skin_component;
  array_add(&array_animation_play_list, &new_play);
}

void play_animation_list() {
  for (int i = 0; i < array_animation_play_list.count; i++) {
    PEAnimationPlay *play = array_get(&array_animation_play_list, i);
    Animation *animation = play->anim;
    if (animation->time <= animation->end) {
      play_animation(play->skin, animation);
#ifdef EDITOR
// update_vertex_bones_gizmos = true;
#endif
    } else {
      if (animation->loop)
        animation->time = 0;
    }
  }
}
