/*Created by pavon on 2019/07/17 */
#ifndef ENGINE_ANIMATION_H
#define ENGINE_ANIMATION_H

#define PATH_TYPE_NULL 0
#define PATH_TYPE_TRANSLATION 1
#define PATH_TYPE_ROTATION 2

#include "../skeletal.h"
#include "../components/components.h"
#include <engine/components/skinned_mesh_component.h>
#include "../array.h"
typedef struct AnimationSampler{
    Array inputs;//int
    Array outputs;//vec3 or vec4
}AnimationSampler;

typedef struct AnimationChannel {
    unsigned short int path_type; 
    Node* node;
    AnimationSampler sampler;
}AnimationChannel;

typedef struct Animation{
    float time;
    bool loop;
    char name[20];
    Array channels;//AnimationChannel
    float start;
    float end; 
}Animation;

typedef struct PEAnimationPlay{
	Animation* anim;
	SkinnedMeshComponent* skin;
}PEAnimationPlay;

Array array_animation_play_list;

void play_animation_by_name(SkinnedMeshComponent* skin_component , const char* name, bool loop);

void play_animation_list();

void pe_anim_nodes_update(SkinnedMeshComponent*);

#ifdef EDITOR
bool update_vertex_bones_gizmos;
#endif

#endif // !ENGINE_ANIMATION_H
