#include "HLOD_factory.h"
#include "editor.h"

#include <editor/gizmos.h>
#include <engine/Math/math.h>
#include "texture_factory.h"

#include <editor/project_manager.h>

#define SAVED_DATA_COUNT 20
vec2 UV_tranlation_offset;
void *saved_vertex_data[SAVED_DATA_COUNT];
PModel *saved_vertex_model[SAVED_DATA_COUNT];
int saved_model_data_count = 0;

HLODBoxComponent *current_HLOD_box_component;

void atlas_resize_UV(PModel *model) {
  PModel new_model;
  memset(&new_model, 0, sizeof(PModel));
  array_init(&new_model.vertex_array, sizeof(PVertex),
             model->vertex_array.count);
  void *data = new_model.vertex_array.data;
  duplicate_model_data(&new_model, model);

  new_model.vertex_array.data = data;
  memcpy(new_model.vertex_array.data, model->vertex_array.data,
         model->vertex_array.actual_bytes_size);

  translate_UV(VEC3(UV_tranlation_offset[0], UV_tranlation_offset[1], 0),
               &new_model, (vec2){0, 0});
  UV_tranlation_offset[0] += 1;
  // UV_tranlation_offset[1] = 1;

  scale_UV(0.5, &new_model, (vec2){0, 0});

  saved_vertex_model[saved_model_data_count] = model;

  saved_vertex_data[saved_model_data_count] = model->vertex_array.data;
  saved_model_data_count++;

  model->vertex_array.data = new_model.vertex_array.data;
}

void check_is_inside(ComponentDefinition *component_definition) {

  if (component_definition->type == STATIC_MESH_COMPONENT) {
    StaticMeshComponent *mesh = component_definition->data;

    if (glm_aabb_contains(current_HLOD_box_component->bounding_box,
                          mesh->bounding_box)) {

      array_add(&array_elements_for_HLOD_generation,
                &component_definition->parent);
      LOG("%s\n", component_definition->parent->name);
      unsigned int *id = array_get(&mesh->meshes, mesh->meshes.count - 1);
      PModel *model = array_get(actual_model_array, *id);
    }
  }
}

float sphere_volume_overlap(Sphere *sphere01, Sphere *sphere02,
                            float fill_factor01, float fill_factor02) {
  if (!sphere_intersect_with_sphere(sphere01, sphere02)) {
    return 0;
  }

  if (sphere_inside_sphere(sphere01, sphere02))
    return get_sphere_volume(sphere01);

  if (sphere_inside_sphere(sphere02, sphere01))
    return get_sphere_volume(sphere02);

  if (sphere_equals(sphere01, sphere02)) {
    return get_sphere_volume(sphere01);
  }

  float distance = glm_vec3_distance(sphere01->center, sphere02->center);

  float height_cap_sphere01 = sphere_get_height_cap(sphere01, distance);
  float height_cap_sphere02 = sphere_get_height_cap(sphere02, distance);
  if (height_cap_sphere01 <= 0 || height_cap_sphere02 <= 0)
    return 0;

  float distance_square = distance * distance;

  float part01 = (sphere01->radius + sphere02->radius) *
                     (sphere01->radius + sphere02->radius) -
                 distance_square;

  float part02 = distance_square - ((sphere01->radius - sphere02->radius) *
                                    (sphere01->radius - sphere02->radius));

  float cap_radius = sqrt(part01 * part02) / 2 * distance;

  float PI_divided_six = M_PI / 6.0f;
  float volumen_cap01 = PI_divided_six *
                        (3 * (cap_radius * cap_radius) +
                         height_cap_sphere01 * height_cap_sphere01) *
                        height_cap_sphere01;
  float volumen_cap02 = PI_divided_six *
                        (3 * (cap_radius * cap_radius) +
                         height_cap_sphere02 * height_cap_sphere02) *
                        height_cap_sphere02;

  float volume_overlap =
      fill_factor01 * volumen_cap01 + fill_factor02 * volumen_cap02;
  if (volume_overlap < 0) {
    LOG("Negative result\n");
    return 0;
  }
  return volume_overlap;
}

float calculate_fill_factor(Sphere *sphere01, Sphere *sphere02,
                            float fill_factor_sphere01,
                            float fill_factor_sphere02) {
  if (sphere_inside_sphere(sphere02, sphere01)) {
    return get_sphere_volume(sphere01);
  }

  float overlap_volume = sphere_volume_overlap(
      sphere01, sphere02, fill_factor_sphere01, fill_factor_sphere02);

  Sphere merge_sphere;
  memset(&merge_sphere, 0, sizeof(Sphere));
  sphere_merge(sphere01, sphere02, &merge_sphere);

  float fill_dot_volume = fill_factor_sphere01 * get_sphere_volume(sphere01);
  float fill_dot_volume02 = fill_factor_sphere02 * get_sphere_volume(sphere02);
  float dividend = fill_dot_volume + fill_dot_volume02 - overlap_volume;
  float merge_sphres_volumes = get_sphere_volume(&merge_sphere);

  float result = dividend / merge_sphres_volumes;
  return result;
}

int short_cluster(const void *cluster01, const void *cluster02) {
  HLODCluster cluster;
  cluster.cost =
      ((HLODCluster *)cluster01)->cost - ((HLODCluster *)cluster02)->cost;
  return ((HLODCluster *)cluster01)->cost - ((HLODCluster *)cluster02)->cost;
}

bool element_inside_cluster_element_array(Element *element,
                                          HLODCluster *cluster) {
  for (int i = 0; i < cluster->elements.count; i++) {
    Element **ppElement = array_get(&cluster->elements, i);
    Element *array_element = ppElement[0];
    if (array_element->id == element->id)
      return true;
  }

  return false;
}

void cluster_merge(HLODCluster *cluster01, HLODCluster *cluster02) {
  Element *elements[50];
  memset(elements, 0, sizeof(elements));
  int count = 0;
  for (int e = 0; e < cluster01->elements.count; e++) {
    Element **ppElement = array_get(&cluster01->elements, e);
    Element *element01 = ppElement[0];
    if (element_inside_cluster_element_array(element01, cluster02))
      continue;

    elements[count] = element01;
    count++;
  }

  for (u8 i = 0; i < count; i++) {
    array_add(&cluster02->elements, &elements[i]);
  }
}
bool check_if_cluster_contens_same_element(HLODCluster *cluster01,
                                           HLODCluster *cluster02) {
  for (int e = 0; e < cluster01->elements.count; e++) {
    Element **ppElement = array_get(&cluster01->elements, e);
    Element *element01 = ppElement[0];

    if (element_inside_cluster_element_array(element01, cluster02))
      return true;
  }
  return false;
}

int merge_pairs(float bounding_value, float max_cost) {
  int merged = 0;

  LOG("Merging spheres edges...\n");
  LOG("HLOD Cluster init count = %i\n", HLOD_generated_cluster.count);
  for (int i = 0; i < HLOD_generated_cluster.count; i++) {
    HLODCluster *cluster = array_get(&HLOD_generated_cluster, i);
    if (cluster->is_valid) {
      for (int j = 0; j < i; j++) {
        HLODCluster *cluster_for_merge = array_get(&HLOD_generated_cluster, j);

        if (cluster_for_merge->is_valid) {
          if (!check_if_cluster_contens_same_element(cluster,
                                                     cluster_for_merge))
            continue;

          Sphere new_bounding_sphere;
          memset(&new_bounding_sphere, 0, sizeof(Sphere));
          sphere_merge(&cluster->bounding_sphere,
                       &cluster_for_merge->bounding_sphere,
                       &new_bounding_sphere);

          float fill_factor = calculate_fill_factor(
              &cluster->bounding_sphere, &cluster_for_merge->bounding_sphere,
              cluster->fill_factor, cluster_for_merge->fill_factor);

          float cost = cube(new_bounding_sphere.radius) / fill_factor;
          if (cost <= max_cost) {
            cluster_merge(cluster, cluster_for_merge);

            HLODCluster new_cluster;
            memset(&new_cluster, 0, sizeof(HLODCluster));
            new_cluster.is_valid = true;
            new_cluster.bounding_sphere = new_bounding_sphere;

            cluster_for_merge->fill_factor = new_cluster.fill_factor;
            cluster_for_merge->bounding_sphere = new_cluster.bounding_sphere;
            cluster_for_merge->cost = new_cluster.cost;
            cluster->is_valid = false;

            merged++;
            break;
          } else {
            // cluster - cluster_to_merge
            LOG("NOt merge\n");
            cluster->is_valid = false;
          }
        }
      }
    }
  }

  for (int i = 0; i < HLOD_generated_cluster.count; i++) {
    HLODCluster *cluster = array_get(&HLOD_generated_cluster, i);
    if (cluster->is_valid) {
      LOG("Cluster active, %i\n", i);
      for (int o = 0; o < cluster->elements.count; o++) {
        Element **ppElement = array_get(&cluster->elements, o);
        Element *element = ppElement[0];
        LOG("Element name: %s\n", element->name);
      }
    }
  }

  return merged;
}

void compute_bounding_sphere_for_every_mesh(float bounding_value) {

  int percentage = 50;
  float max_cost = cube(bounding_value) / percentage;

  for (int i = 0; i < actual_elements_array->count; i++) {
    Element *element01 = array_get(actual_elements_array, i);
    StaticMeshComponent *mesh01 =
        get_component_from_element(element01, STATIC_MESH_COMPONENT);
    if (!mesh01)
      continue;

    for (int j = i + 1; j < actual_elements_array->count; j++) {
      Element *element02 = array_get(actual_elements_array, j);
      StaticMeshComponent *mesh02 =
          get_component_from_element(element02, STATIC_MESH_COMPONENT);
      if (!mesh02)
        continue;

      LOG("Procesing %s , %s\n", element01->name, element02->name);

      Sphere sphere01;
      Sphere sphere02;
      memset(&sphere01, 0, sizeof(Sphere));
      memset(&sphere02, 0, sizeof(Sphere));

      sphere01.radius = glm_aabb_radius(mesh01->bounding_box);
      glm_vec3_copy(mesh01->center, sphere01.center);

      sphere02.radius = glm_aabb_radius(mesh02->bounding_box);
      glm_vec3_copy(mesh02->center, sphere02.center);

      Sphere cluster_sphere;
      memset(&cluster_sphere, 0, sizeof(Sphere));
      sphere_merge(&sphere01, &sphere02, &cluster_sphere);

      if (!sphere_intersect_with_sphere(&sphere01, &sphere02))
        continue;

      float fill_factor = calculate_fill_factor(&sphere01, &sphere02, 1.f, 1.f);
      float cost = cube(cluster_sphere.radius) / fill_factor;

      LOG("Fill factor= %f , cost= %f\n", fill_factor, cost);

      if (cost <= max_cost || sphere_inside_sphere(&sphere01, &sphere02) ||
          sphere_inside_sphere(&sphere02, &sphere01)) {
        HLODCluster cluster;
        memset(&cluster, 0, sizeof(HLODCluster));
        cluster.is_valid = true;
        cluster.bounding_sphere = cluster_sphere;

        array_init(&cluster.elements, sizeof(Element *), 50);
        cluster.elements.isPointerToPointer = true;
        array_add(&cluster.elements, &element01);
        array_add(&cluster.elements, &element02);

        array_add(&HLOD_generated_cluster, &cluster);
      }
    }
  }

  for (int i = 0; i < HLOD_generated_cluster.count; i++) {
    HLODCluster *cluster = array_get(&HLOD_generated_cluster, i);
    LOG("Before, Cluster %i: with %f\n", i, cluster->cost);
  }

  qsort(HLOD_generated_cluster.data, HLOD_generated_cluster.count,
        sizeof(HLODCluster), short_cluster);

  for (int i = 0; i < HLOD_generated_cluster.count; i++) {
    HLODCluster *cluster = array_get(&HLOD_generated_cluster, i);
    LOG("After short: %f\n", cluster->cost);
  }
  HLODCluster *last_cluster =
      array_get(&HLOD_generated_cluster, HLOD_generated_cluster.count - 1);
  if (!last_cluster)
    return;
  if (merge_pairs(bounding_value, last_cluster->cost) == 0) {
    // create cluster spheres pair
    for (int i = 0; i < HLOD_generated_cluster.count; i++) {
      HLODCluster *cluster = array_get(&HLOD_generated_cluster, i);
      if (cluster && cluster->is_valid) {
        for (int j = i + 1; j < HLOD_generated_cluster.count; j++) {
          HLODCluster *cluster2 = array_get(&HLOD_generated_cluster, j);
          if (cluster2 && cluster2->is_valid) {
            HLODCluster new_cluster;
            memset(&new_cluster, 0, sizeof(HLODCluster));
            new_cluster.is_valid = true;
            new_cluster.fill_factor = calculate_fill_factor(
                &cluster->bounding_sphere, &cluster2->bounding_sphere,
                cluster->fill_factor, cluster2->fill_factor);

            Sphere merged_cluster_sphere;
            memset(&merged_cluster_sphere, 0, sizeof(Sphere));
            sphere_merge(&cluster->bounding_sphere, &cluster2->bounding_sphere,
                         &merged_cluster_sphere);

            new_cluster.bounding_sphere = merged_cluster_sphere;

            new_cluster.cost = cube(new_cluster.bounding_sphere.radius) /
                               new_cluster.fill_factor;

            if (new_cluster.cost <= max_cost) {
              cluster_merge(cluster, cluster2);

              cluster2->fill_factor = new_cluster.fill_factor;
              cluster2->bounding_sphere = new_cluster.bounding_sphere;
              cluster2->cost = new_cluster.cost;
              cluster->is_valid = false;
            }
          }
        }
      }
    }
  }
}

char export_folder[] = "Content/HLOD/";
char format[] = ".gltf";
void export_actives_cluster() {
  for (int i = 0; i < HLOD_generated_cluster.count; i++) {
    HLODCluster *cluster = array_get(&HLOD_generated_cluster, i);
    if (cluster->is_valid) {
      cluster->id = i;
      array_clean(&array_elements_for_HLOD_generation);
      for (u8 j = 0; j < cluster->elements.count; j++) {
        Element **ppElement = array_get(&cluster->elements, j);
        Element *element = ppElement[0];
        array_add(&array_elements_for_HLOD_generation, &element);

        StaticMeshComponent *mesh =
            get_component_from_element(element, STATIC_MESH_COMPONENT);
        u32 *id = array_get(&mesh->meshes, mesh->meshes.count - 1);
        PModel *model = array_get(actual_model_array, *id);
        atlas_resize_UV(model);
      }

      char final_export_name[strlen(pavon_the_game_project_folder) +
                             strlen(export_folder) + strlen(format) + 20];
      sprintf(final_export_name, "%s%s%s%i%s", pavon_the_game_project_folder,
              export_folder, "HLOD_out", i, format);
      if (data_export_models_in_array(&array_elements_for_HLOD_generation,
                                      final_export_name) == -1) {
        LOG("Not exported\n");
        return;
      }
      for (u8 i = 0; i < saved_model_data_count; i++) {
        saved_vertex_model[i]->vertex_array.data = saved_vertex_data[i];
      }

      char texture_name[strlen(pavon_the_game_project_folder) + 30];
      sprintf(texture_name, "%s%s%i", pavon_the_game_project_folder,
              "Content/HLOD/HLOD_texture%i.png", i);
      merge_textures(texture_name);
      saved_model_data_count = 0;

      UV_tranlation_offset[0] = 0;
      UV_tranlation_offset[1] = -2;

      array_clean(&array_elements_for_HLOD_generation);
    }
  }
}

#define CLUSTER_SIZE 20
void generate_HLODS(bool export) {
  array_clean(&HLOD_generated_cluster);
  UV_tranlation_offset[0] = 0;
  UV_tranlation_offset[1] = -2;

  if (!array_elements_for_HLOD_generation.initialized) {
    array_init(&array_elements_for_HLOD_generation, sizeof(Element *), 500);
    array_elements_for_HLOD_generation.isPointerToPointer = true;
  }
  array_clean(&array_elements_for_HLOD_generation);

  for (int i = 0; i < HLOD_cluster_count; i++) {
    Element *elmeent = HLOD_clusters[i];
    current_HLOD_box_component =
        get_component_from_element(elmeent, COMPONENT_HLOD_BOX);
    for_each_element_components(check_is_inside);
  }

  if (!HLOD_generated_cluster.initialized) {
    array_init(&HLOD_generated_cluster, sizeof(HLODCluster), 200);
  }

  compute_bounding_sphere_for_every_mesh(CLUSTER_SIZE);

  if (!export)
    return;

  export_actives_cluster();

  for (u8 i = 0; i < HLOD_generated_cluster.count; i++) {
    HLODCluster *cluster = array_get(&HLOD_generated_cluster, i);
    if (cluster->is_valid)
      editor_add_HLOD_element(cluster);
  }
}
Sphere sphere01;
Sphere sphere02;

void HLODs_generated_debug() {
  Sphere main_cluster_size;
  main_cluster_size.radius = CLUSTER_SIZE;
  glm_vec3_copy(VEC3(0, 0, 0), main_cluster_size.center);
  gizmos_boanding_sphere_draw(&main_cluster_size, (vec4){1, 1, 0, 1});

  for (u8 i = 0; i < HLOD_generated_cluster.count; i++) {
    HLODCluster *cluster = array_get(&HLOD_generated_cluster, i);
    if (cluster->is_valid) {
      gizmos_boanding_sphere_draw(&cluster->bounding_sphere,
                                  (vec4){1, 0, 0, 1});
    }
  }
  /*     sphere01.radius = 1;
  glm_vec3_copy(VEC3(5.23,0,0),sphere01.center);
  sphere02.radius = 1;
  glm_vec3_copy(VEC3(8.23,0,0),sphere02.center);

  gizmos_boanding_sphere_draw(&sphere01,(vec4){1,1,1,1});
  gizmos_boanding_sphere_draw(&sphere02,(vec4){1,1,1,1});

  Sphere merge;
  sphere_merge(&sphere01,&sphere02,&merge);

  gizmos_boanding_sphere_draw(&merge,(vec4){1,1,1,1}); */
}
