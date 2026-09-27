#include "terrain_collision.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define CELL_SIZE 4.0f
#define CELLS_ACROSS_MAX 128

//a ray that is this close to the plane of a triangle is running along it
#define PARALLEL_EPSILON 1e-8f

static void triangle_normal(const PCollisionMesh *mesh, u32 triangle,
                            vec3 normal) {
  const float *a = mesh->positions[mesh->triangles[triangle][0]];
  const float *b = mesh->positions[mesh->triangles[triangle][1]];
  const float *c = mesh->positions[mesh->triangles[triangle][2]];
  vec3 ab, ac;

  glm_vec3_sub((float *)b, (float *)a, ab);
  glm_vec3_sub((float *)c, (float *)a, ac);
  glm_vec3_cross(ab, ac, normal);
  glm_vec3_normalize(normal);
}

static bool is_degenerate(const float *a, const float *b, const float *c) {
  vec3 ab, ac, normal;

  glm_vec3_sub((float *)b, (float *)a, ab);
  glm_vec3_sub((float *)c, (float *)a, ac);
  glm_vec3_cross(ab, ac, normal);
  return glm_vec3_norm2(normal) < 1e-12f;
}

static bool copy_triangles(const PBuilding *source, PCollisionMesh *mesh) {
  mesh->vertex_count = source->vertex_count;
  mesh->positions = malloc(source->vertex_count * sizeof(*mesh->positions));
  mesh->triangles = malloc((source->index_count / 3 + 1) * sizeof(*mesh->triangles));
  if (mesh->positions == NULL || mesh->triangles == NULL)
    return false;

  for (u32 i = 0; i < source->vertex_count; i++)
    memcpy(mesh->positions[i], source->vertices[i].position,
           sizeof(mesh->positions[i]));

  for (u32 i = 0; i + 2 < source->index_count; i += 3) {
    const u32 *corners = &source->indices[i];

    if (is_degenerate(mesh->positions[corners[0]], mesh->positions[corners[1]],
                      mesh->positions[corners[2]]))
      continue;
    memcpy(mesh->triangles[mesh->triangle_count++], corners,
           sizeof(mesh->triangles[0]));
  }
  return mesh->triangle_count > 0;
}

static void triangle_box(const PCollisionMesh *mesh, u32 triangle, float *low,
                         float *high) {
  low[0] = low[1] = 1e30f;
  high[0] = high[1] = -1e30f;

  for (int corner = 0; corner < 3; corner++) {
    const float *position = mesh->positions[mesh->triangles[triangle][corner]];

    for (int axis = 0; axis < 2; axis++) {
      low[axis] = fminf(low[axis], position[axis]);
      high[axis] = fmaxf(high[axis], position[axis]);
    }
  }
}

//the cells a box on the plane touches, kept inside the grid. false if none
static bool cells_of_box(const PCollisionMesh *mesh, const float *low,
                         const float *high, u32 *first_column,
                         u32 *last_column, u32 *first_row, u32 *last_row) {
  float first[2], last[2];
  u32 limit[2] = {mesh->columns, mesh->rows};
  u32 from[2], to[2];

  for (int axis = 0; axis < 2; axis++) {
    first[axis] = floorf((low[axis] - mesh->origin[axis]) / mesh->cell_size);
    last[axis] = floorf((high[axis] - mesh->origin[axis]) / mesh->cell_size);
    if (last[axis] < 0 || first[axis] >= (float)limit[axis])
      return false;

    from[axis] = first[axis] < 0 ? 0 : (u32)first[axis];
    to[axis] = last[axis] >= (float)limit[axis] ? limit[axis] - 1 : (u32)last[axis];
  }

  *first_column = from[0];
  *last_column = to[0];
  *first_row = from[1];
  *last_row = to[1];
  return true;
}

static void make_grid(PCollisionMesh *mesh) {
  float low[2] = {1e30f, 1e30f};
  float high[2] = {-1e30f, -1e30f};

  for (u32 i = 0; i < mesh->vertex_count; i++)
    for (int axis = 0; axis < 2; axis++) {
      low[axis] = fminf(low[axis], mesh->positions[i][axis]);
      high[axis] = fmaxf(high[axis], mesh->positions[i][axis]);
    }

  float across = fmaxf(high[0] - low[0], high[1] - low[1]);
  mesh->cell_size = fmaxf(CELL_SIZE, across / CELLS_ACROSS_MAX);
  mesh->origin[0] = low[0];
  mesh->origin[1] = low[1];
  mesh->columns = (u32)((high[0] - low[0]) / mesh->cell_size) + 1;
  mesh->rows = (u32)((high[1] - low[1]) / mesh->cell_size) + 1;
}

//counted once to size each cell's list and then again to fill it
static void fill_cells(PCollisionMesh *mesh, bool fill) {
  u32 cells = mesh->columns * mesh->rows;
  u32 *next = calloc(cells + 1, sizeof(u32));

  for (u32 t = 0; t < mesh->triangle_count; t++) {
    float low[2], high[2];
    u32 c0, c1, r0, r1;

    triangle_box(mesh, t, low, high);
    cells_of_box(mesh, low, high, &c0, &c1, &r0, &r1);

    for (u32 row = r0; row <= r1; row++)
      for (u32 column = c0; column <= c1; column++) {
        u32 cell = row * mesh->columns + column;

        if (fill)
          mesh->cell_triangles[mesh->cell_start[cell] + next[cell]] = t;
        next[cell]++;
      }
  }

  if (fill == false) {
    mesh->cell_start = calloc(cells + 1, sizeof(u32));
    for (u32 i = 0; i < cells; i++)
      mesh->cell_start[i + 1] = mesh->cell_start[i] + next[i];
    mesh->cell_triangles =
        malloc((mesh->cell_start[cells] + 1) * sizeof(u32));
  }
  free(next);
}

bool pe_collision_mesh_build(const PBuilding *source, PCollisionMesh *mesh) {
  memset(mesh, 0, sizeof(*mesh));

  if (copy_triangles(source, mesh) == false) {
    pe_collision_mesh_free(mesh);
    return false;
  }

  make_grid(mesh);
  fill_cells(mesh, false);
  fill_cells(mesh, true);
  return true;
}

void pe_collision_mesh_free(PCollisionMesh *mesh) {
  free(mesh->positions);
  free(mesh->triangles);
  free(mesh->cell_start);
  free(mesh->cell_triangles);
  memset(mesh, 0, sizeof(*mesh));
}

//how far along the ray a triangle is met, from either side. false if it is not
//met, or is met behind the start
static bool ray_meets_triangle(const PCollisionMesh *mesh, u32 triangle,
                               const vec3 origin, const vec3 direction,
                               float *distance) {
  const float *a = mesh->positions[mesh->triangles[triangle][0]];
  const float *b = mesh->positions[mesh->triangles[triangle][1]];
  const float *c = mesh->positions[mesh->triangles[triangle][2]];
  vec3 ab, ac, pvec, tvec, qvec;

  glm_vec3_sub((float *)b, (float *)a, ab);
  glm_vec3_sub((float *)c, (float *)a, ac);
  glm_vec3_cross((float *)direction, ac, pvec);

  float determinant = glm_vec3_dot(ab, pvec);
  if (fabsf(determinant) < PARALLEL_EPSILON)
    return false;

  glm_vec3_sub((float *)origin, (float *)a, tvec);
  float u = glm_vec3_dot(tvec, pvec) / determinant;
  if (u < 0 || u > 1)
    return false;

  glm_vec3_cross(tvec, ab, qvec);
  float v = glm_vec3_dot((float *)direction, qvec) / determinant;
  if (v < 0 || u + v > 1)
    return false;

  *distance = glm_vec3_dot(ac, qvec) / determinant;
  return *distance >= 0;
}

bool pe_collision_mesh_floor(const PCollisionMesh *mesh, const vec3 origin,
                             const vec3 down, float max_distance,
                             float *distance) {
  float low[2], high[2];
  for (int axis = 0; axis < 2; axis++) {
    float end = origin[axis] + down[axis] * max_distance;
    low[axis] = fminf(origin[axis], end);
    high[axis] = fmaxf(origin[axis], end);
  }

  u32 c0, c1, r0, r1;
  if (cells_of_box(mesh, low, high, &c0, &c1, &r0, &r1) == false)
    return false;

  bool found = false;
  float nearest = max_distance;

  for (u32 row = r0; row <= r1; row++)
    for (u32 column = c0; column <= c1; column++) {
      u32 cell = row * mesh->columns + column;

      for (u32 i = mesh->cell_start[cell]; i < mesh->cell_start[cell + 1]; i++) {
        u32 triangle = mesh->cell_triangles[i];
        float along;
        vec3 normal;

        if (ray_meets_triangle(mesh, triangle, origin, down, &along) == false ||
            along > nearest)
          continue;

        triangle_normal(mesh, triangle, normal);
        if (fabsf(glm_vec3_dot(normal, (float *)down)) < PE_COLLISION_WALKABLE)
          continue;

        nearest = along;
        found = true;
      }
    }

  *distance = nearest;
  return found;
}

//the point of a triangle nearest to a point, from Ericson's Real-Time Collision
//Detection
static void closest_point_on_triangle(const float *a, const float *b,
                                      const float *c, const vec3 p,
                                      vec3 closest) {
  vec3 ab, ac, ap;
  glm_vec3_sub((float *)b, (float *)a, ab);
  glm_vec3_sub((float *)c, (float *)a, ac);
  glm_vec3_sub((float *)p, (float *)a, ap);

  float d1 = glm_vec3_dot(ab, ap), d2 = glm_vec3_dot(ac, ap);
  if (d1 <= 0 && d2 <= 0) {
    glm_vec3_copy((float *)a, closest);
    return;
  }

  vec3 bp;
  glm_vec3_sub((float *)p, (float *)b, bp);
  float d3 = glm_vec3_dot(ab, bp), d4 = glm_vec3_dot(ac, bp);
  if (d3 >= 0 && d4 <= d3) {
    glm_vec3_copy((float *)b, closest);
    return;
  }

  float vc = d1 * d4 - d3 * d2;
  if (vc <= 0 && d1 >= 0 && d3 <= 0) {
    glm_vec3_scale(ab, d1 / (d1 - d3), closest);
    glm_vec3_add((float *)a, closest, closest);
    return;
  }

  vec3 cp;
  glm_vec3_sub((float *)p, (float *)c, cp);
  float d5 = glm_vec3_dot(ab, cp), d6 = glm_vec3_dot(ac, cp);
  if (d6 >= 0 && d5 <= d6) {
    glm_vec3_copy((float *)c, closest);
    return;
  }

  float vb = d5 * d2 - d1 * d6;
  if (vb <= 0 && d2 >= 0 && d6 <= 0) {
    glm_vec3_scale(ac, d2 / (d2 - d6), closest);
    glm_vec3_add((float *)a, closest, closest);
    return;
  }

  float va = d3 * d6 - d5 * d4;
  if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) {
    vec3 bc;
    glm_vec3_sub((float *)c, (float *)b, bc);
    glm_vec3_scale(bc, (d4 - d3) / ((d4 - d3) + (d5 - d6)), closest);
    glm_vec3_add((float *)b, closest, closest);
    return;
  }

  float denominator = 1.0f / (va + vb + vc);
  vec3 by_b, by_c;
  glm_vec3_scale(ab, vb * denominator, by_b);
  glm_vec3_scale(ac, vc * denominator, by_c);
  glm_vec3_add((float *)a, by_b, closest);
  glm_vec3_add(closest, by_c, closest);
}

//moves the centre out of one triangle, along the plane at right angles to up
static bool push_out_of_triangle(const PCollisionMesh *mesh, u32 triangle,
                                 vec3 centre, float radius, const vec3 up) {
  vec3 normal;
  triangle_normal(mesh, triangle, normal);
  if (fabsf(glm_vec3_dot(normal, (float *)up)) >= PE_COLLISION_WALKABLE)
    return false;

  vec3 closest, away;
  closest_point_on_triangle(mesh->positions[mesh->triangles[triangle][0]],
                            mesh->positions[mesh->triangles[triangle][1]],
                            mesh->positions[mesh->triangles[triangle][2]],
                            centre, closest);
  glm_vec3_sub(centre, closest, away);

  float distance = glm_vec3_norm(away);
  if (distance >= radius)
    return false;

  //the centre is on the triangle itself, so the way out is its normal
  if (distance < 1e-6f)
    glm_vec3_copy(normal, away);
  else
    glm_vec3_scale(away, 1.0f / distance, away);

  glm_vec3_muladds((float *)up, -glm_vec3_dot(away, (float *)up), away);
  if (glm_vec3_norm2(away) < 1e-12f)
    return false;
  glm_vec3_normalize(away);

  glm_vec3_muladds(away, radius - distance, centre);
  return true;
}

bool pe_collision_mesh_push_out(const PCollisionMesh *mesh, vec3 centre,
                                float radius, const vec3 up) {
  float low[2] = {centre[0] - radius, centre[1] - radius};
  float high[2] = {centre[0] + radius, centre[1] + radius};
  u32 c0, c1, r0, r1;

  if (cells_of_box(mesh, low, high, &c0, &c1, &r0, &r1) == false)
    return false;

  bool moved = false;
  for (u32 row = r0; row <= r1; row++)
    for (u32 column = c0; column <= c1; column++) {
      u32 cell = row * mesh->columns + column;

      for (u32 i = mesh->cell_start[cell]; i < mesh->cell_start[cell + 1]; i++)
        moved |= push_out_of_triangle(mesh, mesh->cell_triangles[i], centre,
                                      radius, up);
    }
  return moved;
}
