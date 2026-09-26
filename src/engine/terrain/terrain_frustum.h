#ifndef PE_TERRAIN_FRUSTUM_H
#define PE_TERRAIN_FRUSTUM_H

#include <cglm/cglm.h>
#include <stdbool.h>

#define PE_TERRAIN_FRUSTUM_PLANES 6

static inline void pe_terrain_matrix_row(const mat4 matrix, int row,
                                         vec4 out) {
  for (int i = 0; i < 4; i++)
    out[i] = matrix[i][row];
}

//INFO the planes come from the rows of the projection times the view. the
//depth range is 0 to 1 here, so the near plane is the third row alone, where
//-1 to 1 would make it the fourth row plus the third. each plane points into
//the frustum, so a point inside has a positive distance from all six
static inline void pe_terrain_frustum_planes(
    const mat4 view_projection, vec4 planes[PE_TERRAIN_FRUSTUM_PLANES]) {
  vec4 x, y, z, w;
  pe_terrain_matrix_row(view_projection, 0, x);
  pe_terrain_matrix_row(view_projection, 1, y);
  pe_terrain_matrix_row(view_projection, 2, z);
  pe_terrain_matrix_row(view_projection, 3, w);

  glm_vec4_add(w, x, planes[0]);
  glm_vec4_sub(w, x, planes[1]);
  glm_vec4_add(w, y, planes[2]);
  glm_vec4_sub(w, y, planes[3]);
  glm_vec4_copy(z, planes[4]);
  glm_vec4_sub(w, z, planes[5]);

  for (int i = 0; i < PE_TERRAIN_FRUSTUM_PLANES; i++)
    glm_vec4_scale(planes[i], 1 / glm_vec3_norm(planes[i]), planes[i]);
}

//a sphere is given as its centre in xyz and its radius in w
static inline bool
pe_terrain_sphere_in_frustum(vec4 planes[PE_TERRAIN_FRUSTUM_PLANES],
                             const vec4 sphere) {
  for (int i = 0; i < PE_TERRAIN_FRUSTUM_PLANES; i++)
    if (glm_vec3_dot(planes[i], (float *)sphere) + planes[i][3] < -sphere[3])
      return false;
  return true;
}

static inline bool pe_terrain_sphere_within(const vec4 sphere,
                                            const vec4 point, float distance) {
  return glm_vec3_distance((float *)sphere, (float *)point) - sphere[3] <=
         distance;
}

#endif // !PE_TERRAIN_FRUSTUM_H
