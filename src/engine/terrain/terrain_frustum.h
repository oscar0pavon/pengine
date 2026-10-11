#ifndef PE_TERRAIN_FRUSTUM_H
#define PE_TERRAIN_FRUSTUM_H

#include <engine/frustum.h>

#define PE_TERRAIN_FRUSTUM_PLANES PE_FRUSTUM_PLANES

static inline void pe_terrain_frustum_planes(
    const mat4 view_projection, vec4 planes[PE_TERRAIN_FRUSTUM_PLANES]) {
  pe_frustum_planes(view_projection, planes);
}

static inline bool
pe_terrain_sphere_in_frustum(vec4 planes[PE_TERRAIN_FRUSTUM_PLANES],
                             const vec4 sphere) {
  return pe_sphere_in_frustum(planes, sphere);
}

static inline bool pe_terrain_sphere_within(const vec4 sphere,
                                            const vec4 point, float distance) {
  return glm_vec3_distance((float *)sphere, (float *)point) - sphere[3] <=
         distance;
}

#endif // !PE_TERRAIN_FRUSTUM_H
