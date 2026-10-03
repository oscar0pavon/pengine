#include "model.h"
#include "engine/array.h"
#include "renderer/vulkan.h"
#include "renderer/vk_images.h"
#include <vulkan/vulkan_core.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include "stdio.h"

#include <cglm/vec3.h>
#include "file_loader.h"

#include "engine/camera.h"

#include "vertex.h"

#include "renderer/vk_vertex.h"
#include "renderer/descriptor_set.h"
#include "renderer/uniform_buffer.h"

#include "skeletal.h"
#include "animation/animation.h"

cgltf_data *current_data;

//INFO set by pe_vk_load_skin() before the parse below, the same way
//current_data is: pe_node_load() and the skin/animation blocks in
//pe_loader_model_from_memory() only have this one call stack to fill a
//PSkin in, since the cgltf_skin and cgltf_animation they read from are
//freed at the end of it
PSkin *current_skin;


//INFO only the first call sizes the array; the calls after it, one per
//further primitive of the same mesh, add their indices to that same one.
//array_init() cannot be left to refuse the later calls itself - it zeroes
//the Array before testing whether it was already initialized, so its own
//guard never fires and it would start the array over every primitive,
//leaving a mesh with only its last primitive's indices
void pe_loader_mesh_read_accessor_indices(Array *index_array,
                                          cgltf_accessor *accessor) {
  if (index_array->initialized == false) {
    switch (accessor->component_type) {
    case cgltf_component_type_r_8u:
      array_init(index_array, sizeof(u8), accessor->count);
      break;
    case cgltf_component_type_r_16u:
      array_init(index_array, sizeof(unsigned short), accessor->count);
      break;
    case cgltf_component_type_r_32u:
      array_init(index_array, sizeof(unsigned int), accessor->count);
      break;
    default:
      break;
    }
  }

  for (size_t i = 0; i < accessor->count; i++) {
    size_t index = cgltf_accessor_read_index(accessor, i);
    array_add(index_array, &index);
  }
}

void pe_loader_read_accessor(Array* array, cgltf_accessor *accessor, float *out) {
  switch (accessor->type) {
  case cgltf_type_vec2: {

    for (int i = 0; i < accessor->count; i++) {
      cgltf_accessor_read_float(accessor, i, &out[i * 2], 2);
    }
    break;
  }
  case cgltf_type_vec3: {

    for (int i = 0; i < accessor->count; i++) {
      cgltf_accessor_read_float(accessor, i, &out[i * 3], 3);
    }

    break;
  }
  case cgltf_type_vec4: {

    for (int i = 0; i < accessor->count; i++) {
      cgltf_accessor_read_float(accessor, i, &out[i * 4], 4);
    }
    break;
  }
  case cgltf_type_mat4: {
    for (int i = 0; i < accessor->count; i++) {
      cgltf_accessor_read_float(accessor, i, &out[i * 16], 16);
    }
    break;
  }
  case cgltf_type_scalar: {

    for (int i = 0; i < accessor->count; i++) {
      float number;
      cgltf_accessor_read_float(accessor, i, &number, 1);
      array_add(array, &number);
    }

    break;
  }
  }

  switch (accessor->component_type) {
  case cgltf_component_type_r_16:
    /* code */
    break;

  default:
    break;
  }
}

//where a primitive's own vertices start in the array, which is 0 for the first
//primitive of a mesh and past the ones before it for a part
static u32 pe_attribute_base;

void pe_load_attribute(Array* vertex_array, cgltf_attribute *attribute) {
  //INFO a file may carry several UV sets and colour sets, TEXCOORD_1 and on,
  //and Blender writes every UV map of a mesh. only the first of each is what a
  //material's texture reads, and a later one read over it leaves the model
  //sampling a single point
  if ((attribute->type == cgltf_attribute_type_texcoord ||
       attribute->type == cgltf_attribute_type_color) &&
      attribute->index != 0)
    return;

  switch (attribute->type) {
  case cgltf_attribute_type_position: {
    LOG("#### Vertex count: %i\n", (int)attribute->data->count);
    vec3 vertices_position[attribute->data->count];
    ZERO(vertices_position);

    if (pe_attribute_base == 0)
      array_init(vertex_array, sizeof(PVertex), attribute->data->count);

    pe_loader_read_accessor(
        vertex_array, attribute->data,
        (float *)vertices_position); // TODO maybe here we broke something

    for (int i = 0; i < attribute->data->count; i++) {
      PVertex vertex;
      ZERO(vertex);
      glm_vec3_copy(vertices_position[i], vertex.position);
      array_add(vertex_array, &vertex);
    }
    break;
  }
  case cgltf_attribute_type_texcoord: {

    vec2 uvs[attribute->data->count];
    ZERO(uvs);

    pe_loader_read_accessor(vertex_array, attribute->data, (float*)uvs);

    for (int i = 0; i < attribute->data->count; i++) {
      PVertex *vertex = array_get(vertex_array, pe_attribute_base + i);
      vertex->uv[0] = uvs[i][0];
      vertex->uv[1] = uvs[i][1];
    }

    break;
  }
  case cgltf_attribute_type_normal: {
    // LOG("Normal attribute \n");
    vec3 normals[attribute->data->count];
    ZERO(normals);

    pe_loader_read_accessor(vertex_array, attribute->data, (float *)normals);

    for (int i = 0; i < attribute->data->count; i++) {
      PVertex *vertex = array_get(vertex_array, pe_attribute_base + i);
      glm_vec3_copy(normals[i], vertex->normal);
    }

    break;
  }
  //INFO COLOR_0 is a vec3 or a vec4 of floats or normalized integers. the
  //alpha is dropped: a model drawn with vertex colours is opaque
  case cgltf_attribute_type_color: {
    for (int i = 0; i < attribute->data->count; i++) {
      float rgba[4] = {1, 1, 1, 1};
      cgltf_accessor_read_float(attribute->data, i, rgba, 4);
      PVertex *vertex = array_get(vertex_array, pe_attribute_base + i);
      glm_vec3_copy(rgba, vertex->color);
    }
    break;
  }
  //INFO JOINTS_0 is usually unsigned byte or short, never normalized, so
  //cgltf_accessor_read_float() (via cgltf_component_read_float()) hands back
  //the plain index 0..255 as a float rather than scaling it - exactly what
  //int(joint.x) in the skinned vertex shader wants
  case cgltf_attribute_type_joints: {
    vec4 joints[attribute->data->count];
    ZERO(joints);

    pe_loader_read_accessor(vertex_array, attribute->data, (float *)joints);

    for (int i = 0; i < attribute->data->count; i++) {
      PVertex *vertex = array_get(vertex_array, pe_attribute_base + i);
      glm_vec4_copy(joints[i], vertex->joint);
    }

    break;
  }
  case cgltf_attribute_type_weights: {
    vec4 weights[attribute->data->count];
    ZERO(weights);

    pe_loader_read_accessor(vertex_array, attribute->data, (float *)weights);

    for (int i = 0; i < attribute->data->count; i++) {
      PVertex *vertex = array_get(vertex_array, pe_attribute_base + i);
      glm_vec4_copy(weights[i], vertex->weight);
    }

    break;
  }

  } // end switch

  if (attribute->data->has_min) {

    //glm_vec3_copy(attribute->data->min, selected_model->min);
  }
  if (attribute->data->has_max) {

    //glm_vec3_copy(attribute->data->max, selected_model->max);
  }
}

//the accessor decides whether an index is one, two or four bytes wide
static u32 pe_loader_index_at(Array *index_array, u32 position) {
  u32 index = 0;
  memcpy(&index, array_get(index_array, position),
         index_array->element_bytes_size);
  return index;
}

//INFO NORMAL is optional in gltf, and a file without it asks the client to
//calculate flat normals. cube.glb - the chess board, and every square copied
//from it - carries only POSITION and TEXCOORD_0, so without this the normals
//stay zero, normalize() in the fragment shader answers NaN and the board draws
//solid black
static void pe_loader_flat_normals(Array *vertex_array, Array *index_array) {

  if (vertex_array->count == 0 || index_array->count < 3)
    return;

  for (u32 i = 0; i + 2 < index_array->count; i += 3) {

    u32 index[3];
    for (int corner = 0; corner < 3; corner++)
      index[corner] = pe_loader_index_at(index_array, i + corner);

    if (index[0] >= vertex_array->count || index[1] >= vertex_array->count ||
        index[2] >= vertex_array->count)
      continue;

    PVertex *corner[3];
    for (int c = 0; c < 3; c++)
      corner[c] = array_get(vertex_array, index[c]);

    vec3 edge1, edge2, normal;
    glm_vec3_sub(corner[1]->position, corner[0]->position, edge1);
    glm_vec3_sub(corner[2]->position, corner[0]->position, edge2);
    glm_vec3_cross(edge1, edge2, normal);

    if (glm_vec3_norm(normal) == 0)
      continue;

    glm_vec3_normalize(normal);

    for (int c = 0; c < 3; c++)
      glm_vec3_copy(normal, corner[c]->normal);
  }
}

//INFO m22gltf's own format (tools/m22gltf.c in pwow): a primitive carrying a
//geoset writes extras as exactly {"geoset":N}, the classic skin submesh's
//skinSectionId. cgltf leaves extras unparsed, as a byte range into the
//original JSON text, so this reads it straight out of current_data->json
//rather than pulling in a JSON parser for one integer. A primitive with no
//extras (any other glTF, cube.glb among them) has start_offset == end_offset
//and this returns false, which pe_load_mesh takes to mean "not part of a
//group, always keep"
static bool pe_primitive_geoset(cgltf_primitive *primitive, u32 *geoset) {
  cgltf_extras *extras = &primitive->extras;
  if (extras->end_offset <= extras->start_offset)
    return false;
  return sscanf(current_data->json + extras->start_offset, "{\"geoset\":%u",
               geoset) == 1;
}

//0 for a primitive whose extras do not carry a texture_type
static u32 pe_primitive_texture_type(cgltf_primitive *primitive) {
  cgltf_extras *extras = &primitive->extras;
  const char *begin = current_data->json + extras->start_offset;
  const char *end = current_data->json + extras->end_offset;
  const char key[] = "\"texture_type\":";

  u32 type = 0;
  for (const char *c = begin; c + sizeof(key) < end; c++)
    if (strncmp(c, key, sizeof(key) - 1) == 0) {
      sscanf(c + sizeof(key) - 1, "%u", &type);
      break;
    }
  return type;
}

//INFO a model carries every alternative a geoset group has - every hairstyle,
//every cloak, gloves and bare forearms both - since the file is the same one
//WoWee's own DBC-driven equipment code would filter at runtime. Without that
//data this can only pick one default per group (group*100+variant).
//
//group 0 is not a normal choice group: id 0 is the body, always drawn, and
//ids 1.. are alternative scalp meshes layered on top of it (WoWee's own
//entity_spawner.cpp: "group 0 holds the body plus one scalp"). Every
//character needs one of them - even a bald human draws a bald-cap mesh, there
//is no "nothing" option - so one of 1.. has to be picked as a stand-in
//default. It is not simply the lowest id, the way a bare equipment variant
//would be: on the Tauren, id 1 is a real horn style but a 10-vertex stub,
//next to nothing on screen, while id 2 and up are full curved horns of 30-50
//vertices - checked by plotting each id's own triangles, after the first fix
//here picked id 1 and the horns it added were too small to see. Picking
//whichever member has the most geometry instead treats a near-empty mesh as
//an unlikely thing for a group's true representative to be, on any race.
//
//every other group's own "none" is a real, drawn variant, and it is always at
//variant 00 or 01 (WoWee's kGeosetBareForearms = 401, kGeosetBarePants =
//1301, kGeosetNoCape = 1501: no gloves, no leggings, no cape). When a group's
//lowest exported id already sits at variant 0 or 1, that is its bare state
//and this keeps it. When it does not - this Tauren's facial-hair-shaped
//groups 1, 2 and 3 export only variants 2 and up, because
//CharFacialHairStyles never gives Tauren anything in those slots and a
//variant nobody selects has no blank mesh to export in the first place - the
//group has no way to say "none", so guessing its first real style drew a
//warrior in a mane like a shaman's cloth nobody equipped. Rather than guess,
//the whole group is left off; a caller that does have the DBC's per-character
//variant can still ask a loaded model for that geoset directly, this only
//decides what shows with none of that data
static bool pe_primitive_is_default(cgltf_mesh *mesh, int index) {
  u32 geoset;
  if (!pe_primitive_geoset(&mesh->primitives[index], &geoset))
    return true;
  if (geoset == 0)
    return true;

  u32 group = geoset / 100;

  if (group == 0) {
    cgltf_size best_count = 0;
    int best_index = index;
    for (int i = 0; i < mesh->primitives_count; i++) {
      u32 other;
      if (!pe_primitive_geoset(&mesh->primitives[i], &other) || other == 0 ||
          other / 100 != 0)
        continue;
      if (mesh->primitives[i].indices->count > best_count) {
        best_count = mesh->primitives[i].indices->count;
        best_index = i;
      }
    }
    return index == best_index;
  }

  //the group's true lowest id, over every primitive - not just the ones
  //checked so far - so a group with no bare variant is recognised as such
  //regardless of which of its primitives happens to be asked about first
  u32 group_min = geoset;
  for (int i = 0; i < mesh->primitives_count; i++) {
    u32 other;
    if (i != index && pe_primitive_geoset(&mesh->primitives[i], &other) &&
        other / 100 == group && other < group_min)
      group_min = other;
  }

  if (group_min % 100 > 1)
    return false;
  return geoset == group_min;
}

//INFO a mesh's attributes are read once, from its first primitive, and every
//primitive after that only adds its own indices to the same shared vertex
//array. that is the shape a mesh split by material has - one vertex set,
//one primitive per material, each naming the same POSITION/NORMAL/...
//accessors and differing only in which slice of indices it draws - and it
//is what m22gltf writes for a model whose 70 batches all index the same
//2737 vertices. a mesh whose primitives carried genuinely different vertex
//data would need each one appended with its own vertex range and its
//indices offset past the ranges before it, which this does not do.
//
//a material's baseColorTexture is only ever present when m22gltf's own
//model_texture() (tools/m22gltf.c in pwow) resolved the model's texture slot
//to a real file rather than a runtime-replaceable one (a creature or
//character skin painted in by the game, which a model placed in the world
//has no data for) - so when it is there, the model's own baked-in look is
//already correct and needs nothing from the caller beyond having converted
//the png alongside the glb, the same "prepare_*.sh already put it in data/"
//assumption a building or prop's own texture table relies on
static void pe_load_material_texture(PModel *model, cgltf_material *material) {
  if (material == NULL || !material->has_pbr_metallic_roughness)
    return;

  cgltf_texture *texture =
      material->pbr_metallic_roughness.base_color_texture.texture;
  if (texture == NULL || texture->image == NULL || texture->image->uri == NULL)
    return;

  char path[512];
  snprintf(path, sizeof(path), "data/%s", texture->image->uri);
  pe_load_texture(path, &model->texture);
}

//the directory of the file being loaded, which an image named by a relative
//uri is looked for in
static char pe_model_directory[512];

//a mesh whose primitives each have vertices of their own, as Blender writes a
//mesh of several materials, has to be kept as parts: the primitives of a
//character share one set of vertices and are told apart by geoset instead
static bool pe_mesh_is_parts(cgltf_mesh *mesh) {
  if (mesh->primitives_count < 2)
    return false;

  for (int i = 0; i < mesh->primitives_count; i++) {
    u32 geoset;
    if (pe_primitive_geoset(&mesh->primitives[i], &geoset))
      return false;
  }

  cgltf_accessor *first = NULL;
  for (int i = 0; i < mesh->primitives_count; i++)
    for (int a = 0; a < mesh->primitives[i].attributes_count; a++) {
      cgltf_attribute *attribute = &mesh->primitives[i].attributes[a];
      if (attribute->type != cgltf_attribute_type_position)
        continue;
      if (first == NULL)
        first = attribute->data;
      else if (attribute->data != first)
        return true;
    }
  return false;
}

#define PE_PART_TEXTURES_MAX 64

typedef struct PPartTextures {
  cgltf_image *images[PE_PART_TEXTURES_MAX];
  bool linear[PE_PART_TEXTURES_MAX];
  PTexture textures[PE_PART_TEXTURES_MAX];
  u32 count;

  //what a part has when its material names no texture for a slot
  PTexture white;
  bool has_white;
  PTexture flat_normal;
  bool has_flat_normal;
} PPartTextures;

static bool pe_load_image_texture(cgltf_image *image, bool linear,
                                  PTexture *out) {
  if (image->buffer_view != NULL) {
    u8 *data = (u8 *)image->buffer_view->buffer->data + image->buffer_view->offset;
    u32 size = image->buffer_view->size;
    return (linear ? texture_load_from_memory_linear(out, size, data)
                   : texture_load_from_memory(out, size, data)) != -1;
  }

  if (image->uri == NULL || strncmp(image->uri, "data:", 5) == 0)
    return false;

  char path[1024];
  snprintf(path, sizeof(path), "%s%s", pe_model_directory, image->uri);
  return pe_load_texture(path, out) != -1;
}

//the texture a slot of a part is drawn with, loaded once however many
//materials name it. the slot's own default when it has none
static PTexture pe_part_texture(PPartTextures *loaded, cgltf_texture_view *view,
                                bool linear, bool flat_normal) {
  cgltf_image *image = NULL;
  if (view != NULL && view->texture != NULL)
    image = view->texture->image;

  if (image != NULL) {
    for (u32 i = 0; i < loaded->count; i++)
      if (loaded->images[i] == image && loaded->linear[i] == linear)
        return loaded->textures[i];

    if (loaded->count < PE_PART_TEXTURES_MAX) {
      PTexture texture;
      ZERO(texture);
      if (pe_load_image_texture(image, linear, &texture)) {
        loaded->images[loaded->count] = image;
        loaded->linear[loaded->count] = linear;
        loaded->textures[loaded->count] = texture;
        return loaded->textures[loaded->count++];
      }
    }
  }

  if (flat_normal) {
    if (!loaded->has_flat_normal) {
      ZERO(loaded->flat_normal);
      pe_texture_flat_normal(&loaded->flat_normal);
      loaded->has_flat_normal = true;
    }
    return loaded->flat_normal;
  }

  if (!loaded->has_white) {
    ZERO(loaded->white);
    pe_texture_white(&loaded->white);
    loaded->has_white = true;
  }
  return loaded->white;
}

static void pe_load_part_material(PModelPart *part, cgltf_material *material,
                                  PPartTextures *loaded) {
  part->material.metallic = 1;
  part->material.roughness = 1;
  part->material.normal_scale = 1;
  glm_vec4_zero(part->material.emissive);

  cgltf_texture_view *base = NULL, *metal_rough = NULL, *normal = NULL;
  if (material != NULL) {
    if (material->has_pbr_metallic_roughness) {
      cgltf_pbr_metallic_roughness *pbr = &material->pbr_metallic_roughness;
      part->material.metallic = pbr->metallic_factor;
      part->material.roughness = pbr->roughness_factor;
      base = &pbr->base_color_texture;
      metal_rough = &pbr->metallic_roughness_texture;
    }
    normal = &material->normal_texture;
    if (normal->texture != NULL)
      part->material.normal_scale = normal->scale;
    glm_vec3_copy(material->emissive_factor, part->material.emissive);
  }

  part->texture = pe_part_texture(loaded, base, false, false);
  part->metallic_roughness = pe_part_texture(loaded, metal_rough, true, false);
  part->normal = pe_part_texture(loaded, normal, true, true);
}

//a part's vertices are appended after those of the parts before it, and its
//indices point past them. the colour of its material is multiplied into the
//vertex colours, white where the file has none, so a part without a texture
//draws as its material's colour
static void pe_load_part(PModel *model, cgltf_primitive *primitive,
                         PPartTextures *loaded) {
  u32 base = model->vertex_array.initialized ? model->vertex_array.count : 0;

  bool has_color = false;
  pe_attribute_base = base;
  for (int a = 0; a < primitive->attributes_count; a++) {
    if (primitive->attributes[a].type == cgltf_attribute_type_color)
      has_color = true;
    pe_load_attribute(&model->vertex_array, &primitive->attributes[a]);
  }
  pe_attribute_base = 0;

  vec3 factor = {1, 1, 1};
  if (primitive->material != NULL &&
      primitive->material->has_pbr_metallic_roughness)
    glm_vec3_copy(primitive->material->pbr_metallic_roughness.base_color_factor,
                  factor);

  for (u32 i = base; i < model->vertex_array.count; i++) {
    PVertex *vertex = array_get(&model->vertex_array, i);
    if (!has_color)
      glm_vec3_one(vertex->color);
    glm_vec3_mul(vertex->color, factor, vertex->color);
  }

  //parts are indexed with 32 bits, since the vertices of a car do not fit 16
  if (!model->index_array.initialized)
    array_init(&model->index_array, sizeof(u32), primitive->indices->count);

  PModelPart part;
  ZERO(part);
  part.first_index = model->index_array.count;
  for (size_t i = 0; i < primitive->indices->count; i++) {
    u32 index = base + (u32)cgltf_accessor_read_index(primitive->indices, i);
    array_add(&model->index_array, &index);
  }
  part.index_count = model->index_array.count - part.first_index;
  pe_load_part_material(&part, primitive->material, loaded);

  if (!model->parts.initialized)
    array_init(&model->parts, sizeof(PModelPart), 0);
  array_add(&model->parts, &part);
}

static void pe_load_mesh_parts(PModel *model, cgltf_mesh *mesh) {
  PPartTextures loaded;
  ZERO(loaded);

  for (int i = 0; i < mesh->primitives_count; i++)
    pe_load_part(model, &mesh->primitives[i], &loaded);
}

//reading every primitive's attributes instead, as this used to, rebuilt the
//vertex array per primitive and left only the last primitive's indices,
//because array_init() zeroes an Array before it checks whether it was
//already initialized - so the guard meant to refuse a second init never
//fires and each call silently starts the array over
void pe_load_mesh(PModel *model, cgltf_mesh *mesh) {

  if (mesh->primitives_count == 0)
    return;

  if (pe_mesh_is_parts(mesh)) {
    pe_load_mesh_parts(model, mesh);
    return;
  }

  //once per model: a caller loading its own texture over this later (a
  //player's skin, a creature's DBC texture variation) still wins, since this
  //only fires while the texture slot is still unloaded
  if (!model->texture.gpu_loaded)
    pe_load_material_texture(model, mesh->primitives[0].material);

  bool has_normals = false;
  for (int i = 0; i < mesh->primitives[0].attributes_count; i++) {
    if (mesh->primitives[0].attributes[i].type == cgltf_attribute_type_normal)
      has_normals = true;
    pe_load_attribute(&model->vertex_array, &mesh->primitives[0].attributes[i]);
  }

  //all_indices is sized off whichever accessor it sees first, the same way
  //index_array already is - pe_loader_mesh_read_accessor_indices() does that
  //init itself (guarded on ->initialized, not array_init()'s own broken
  //self-check - see this function's doc comment above). geoset_batches has
  //no such helper, so it needs the same guard written out here, or a second
  //mesh node on this model (multiple mesh-bearing nodes in one glTF scene)
  //would silently start it over and lose every batch already recorded
  if (!model->geoset_batches.initialized)
    array_init(&model->geoset_batches, sizeof(PGeosetBatch), 0);

  for (int i = 0; i < mesh->primitives_count; i++) {
    u32 first_index = model->all_indices.count;
    pe_loader_mesh_read_accessor_indices(&model->all_indices,
                                         mesh->primitives[i].indices);

    PGeosetBatch batch;
    batch.tagged = pe_primitive_geoset(&mesh->primitives[i], &batch.geoset);
    batch.is_default = pe_primitive_is_default(mesh, i);
    batch.texture_type = pe_primitive_texture_type(&mesh->primitives[i]);
    batch.first_index = first_index;
    batch.index_count = model->all_indices.count - first_index;
    array_add(&model->geoset_batches, &batch);

    if (!batch.is_default)
      continue;
    pe_loader_mesh_read_accessor_indices(&model->index_array,
                                         mesh->primitives[i].indices);
  }

  if (!has_normals)
    pe_loader_flat_normals(&model->vertex_array, &model->index_array);

  model->extra_first_index = model->index_array.count;
}

u32 pe_model_default_geosets(PModel *model, u32 *out, u32 out_max) {
  u32 n = 0;
  for (u32 i = 0; i < model->geoset_batches.count; i++) {
    PGeosetBatch *batch = array_get(&model->geoset_batches, (int)i);
    if (!batch->tagged || !batch->is_default)
      continue;
    if (n < out_max)
      out[n] = batch->geoset;
    n++;
  }
  return n;
}

bool pe_model_has_geoset(PModel *model, u32 geoset) {
  for (u32 i = 0; i < model->geoset_batches.count; i++) {
    PGeosetBatch *batch = array_get(&model->geoset_batches, (int)i);
    if (batch->tagged && batch->geoset == geoset)
      return true;
  }
  return false;
}

void pe_model_set_active_geosets(PModel *model, const u32 *geosets,
                                 u32 count) {
  Array rebuilt;
  array_init(&rebuilt, model->all_indices.element_bytes_size,
            (int)model->index_array.count);

  //the primitives of the skin extra go last, so a draw can give them their
  //own texture after the rest
  for (int extra = 0; extra < 2; extra++) {
    if (extra == 1)
      model->extra_first_index = rebuilt.count;

    for (u32 i = 0; i < model->geoset_batches.count; i++) {
      PGeosetBatch *batch = array_get(&model->geoset_batches, (int)i);
      if ((batch->texture_type == PE_TEXTURE_TYPE_SKIN_EXTRA) != (extra == 1))
        continue;

      bool visible = !batch->tagged;
      for (u32 g = 0; !visible && g < count; g++)
        visible = geosets[g] == batch->geoset;
      if (!visible)
        continue;

      for (u32 j = 0; j < batch->index_count; j++)
        array_add(&rebuilt, array_get(&model->all_indices,
                                      (int)(batch->first_index + j)));
    }
  }

  //the old index_array's own backing memory is simply abandoned - the
  //engine arena is a bump allocator with no free (array.c), the same trade
  //this already makes on every array_grow(). fine here: equipment changes
  //are a rare, player-driven event, not a per-frame one
  model->index_array = rebuilt;

  //matches terrain_world.c's own tile-unload convention: wait for the gpu
  //to be done with the buffer being replaced before freeing it, rather than
  //risk a frame still in flight reading it
  vkDeviceWaitIdle(vk_device);
  pe_vk_destroy_buffer(&model->index_buffer);
  model->index_buffer = pe_vk_create_buffer(model->index_array.bytes_size,
                                            model->index_array.data,
                                            VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

//INFO joints are only ever bound skin->joints and pe_load_skin()'s own
//cgltf_joints, never freed early, so this is safe to call from
//pe_load_animations() right after
static Node *pe_find_joint_node(PSkin *skin, cgltf_skin *in_skin,
                                cgltf_node *target) {
  for (u32 i = 0; i < in_skin->joints_count; i++)
    if (in_skin->joints[i] == target)
      return array_get(&skin->joints, i);
  return NULL;
}

//one PSkin per file is all this reads, which is all m22gltf (and every other
//glTF exporter meant for a single rigged character) ever writes
static void pe_load_skin(PSkin *skin, cgltf_skin *in_skin) {
  u32 joint_count = in_skin->joints_count;
  cgltf_node *cgltf_joints[joint_count];

  array_init(&skin->joints, sizeof(Node), joint_count);
  array_init(&skin->inverse_bind_matrices, sizeof(mat4), joint_count);

  for (u32 i = 0; i < joint_count; i++) {
    cgltf_node *in_node = in_skin->joints[i];
    cgltf_joints[i] = in_node;

    Node node;
    ZERO(node);
    node.id = i;
    snprintf(node.name, sizeof(node.name), "%s",
            in_node->name ? in_node->name : "");

    if (in_node->has_translation)
      glm_vec3_copy(in_node->translation, node.translation);
    if (in_node->has_rotation)
      glm_vec4_copy(in_node->rotation, node.rotation);
    else
      glm_quat_identity(node.rotation);

    array_add(&skin->joints, &node);

    mat4 inverse_bind;
    glm_mat4_identity(inverse_bind);
    if (in_skin->inverse_bind_matrices != NULL)
      cgltf_accessor_read_float(in_skin->inverse_bind_matrices, i,
                                (float *)inverse_bind, 16);
    array_add(&skin->inverse_bind_matrices, inverse_bind);
  }

  //INFO parents are only wired up now that every joint has its final,
  //settled place in skin->joints - array_add() above may have moved the
  //array when it grew, and a parent pointer taken before that would dangle
  for (u32 i = 0; i < joint_count; i++) {
    Node *child = array_get(&skin->joints, i);
    child->parent = NULL;

    cgltf_node *parent = cgltf_joints[i]->parent;
    for (u32 p = 0; p < joint_count; p++) {
      if (cgltf_joints[p] == parent) {
        child->parent = array_get(&skin->joints, p);
        break;
      }
    }
  }

  skin->node_uniform.joint_count = joint_count;
}

//translation and rotation channels only: PSkin's Node has no scale, matching
//what m22gltf's bones actually need it for (Blizzard's own tooling, not
//gear or squash-and-stretch effects)
static void pe_load_animations(PSkin *skin, cgltf_data *data) {
  if (data->skins_count == 0)
    return;
  cgltf_skin *in_skin = &data->skins[0];

  array_init(&skin->animations, sizeof(PAnimation), data->animations_count);

  for (u32 a = 0; a < data->animations_count; a++) {
    cgltf_animation *in_animation = &data->animations[a];

    PAnimation animation;
    ZERO(animation);
    snprintf(animation.name, sizeof(animation.name), "%s",
            in_animation->name ? in_animation->name : "");

    array_init(&animation.channels, sizeof(PAnimationChannel),
              in_animation->channels_count);

    for (u32 c = 0; c < in_animation->channels_count; c++) {
      cgltf_animation_channel *in_channel = &in_animation->channels[c];

      unsigned short path_type;
      int floats_per;
      if (in_channel->target_path == cgltf_animation_path_type_translation) {
        path_type = PATH_TYPE_TRANSLATION;
        floats_per = 3;
      } else if (in_channel->target_path ==
                cgltf_animation_path_type_rotation) {
        path_type = PATH_TYPE_ROTATION;
        floats_per = 4;
      } else {
        continue;
      }

      Node *node = pe_find_joint_node(skin, in_skin, in_channel->target_node);
      if (node == NULL)
        continue;

      cgltf_animation_sampler *in_sampler = in_channel->sampler;
      u32 key_count = in_sampler->input->count;

      PAnimationChannel channel;
      ZERO(channel);
      channel.path_type = path_type;
      channel.node = node;

      array_init(&channel.sampler.inputs, sizeof(float), key_count);
      array_init(&channel.sampler.outputs, sizeof(float) * floats_per,
                key_count);

      for (u32 k = 0; k < key_count; k++) {
        float time;
        cgltf_accessor_read_float(in_sampler->input, k, &time, 1);
        array_add(&channel.sampler.inputs, &time);

        float value[4];
        cgltf_accessor_read_float(in_sampler->output, k, value, floats_per);
        array_add(&channel.sampler.outputs, value);
      }

      if (key_count > 0) {
        float last_time = *(float *)array_get_last(&channel.sampler.inputs);
        if (last_time > animation.end)
          animation.end = last_time;
      }

      array_add(&animation.channels, &channel);
    }

    array_add(&skin->animations, &animation);
  }
}

int pe_node_load(PModel* model, cgltf_node *in_cgltf_node) {


  if (in_cgltf_node->mesh == NULL) {

    // LOG("********* No mesh in node");
  }

  if (in_cgltf_node->mesh != NULL) {
    LOG("Loading GLTF mesh\n");
    // check_LOD_names(in_cgltf_node);
    pe_load_mesh(model, in_cgltf_node->mesh);
  }

  if (in_cgltf_node->skin != NULL) {

  }


  if (in_cgltf_node->children_count == 0 && in_cgltf_node->mesh == NULL) {
    return 1;
  }

  for (int i = 0; i < in_cgltf_node->children_count; i++) {
    pe_node_load(model, in_cgltf_node->children[i]);
  }
}

cgltf_result pe_loader_model_from_memory(PModel* model, void *gltf_data, u32 size,
                                         const char *path) {
  cgltf_options options = {0};
  cgltf_data *data = NULL;

  cgltf_result result = cgltf_parse(&options, gltf_data, size, &data);
  if (result != cgltf_result_success)
    return result;

  snprintf(pe_model_directory, sizeof(pe_model_directory), "%s", path);
  char *last_slash = strrchr(pe_model_directory, '/');
  if (last_slash != NULL)
    last_slash[1] = '\0';
  else
    pe_model_directory[0] = '\0';

  current_data = data;

  result = cgltf_load_buffers(&options, data, path);
  if (result != cgltf_result_success)
    return result;

  if (!data || !data->scene) {
    return cgltf_result_invalid_options;
  }


  if (current_skin != NULL && data->skins_count >= 1)
    pe_load_skin(current_skin, &data->skins[0]);

  //  LOG("******************Loading nodes");

  for (int i = 0; i < data->scene->nodes_count; i++) {
    pe_node_load(model, data->scene->nodes[i]);
  }


  if (current_skin != NULL && data->animations_count >= 1)
    pe_load_animations(current_skin, data);

  cgltf_free(data);
  current_data = NULL;

  return result;
}

PModel *pe_vk_load_model(PModel* model, const char *path) {

  pe_load_model_path(model, path);

  model->vertex_buffer = pe_vk_create_buffer(model->vertex_array.bytes_size,
                                             model->vertex_array.data,
                                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  model->index_buffer = pe_vk_create_buffer(model->index_array.bytes_size,
                                            model->index_array.data,
                                            VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

  pe_vk_create_uniform_buffers(model, &main_render_target);
  pe_vk_descriptor_pool_create(model, &main_render_target);

  //INFO a model is not drawable until its descriptor sets exist and point at
  //its uniform buffers - pe_vk_draw_model() binds set[image_index] every draw.
  //the pool alone was not enough and left the sets array empty
  pe_vk_create_descriptor_sets(model, pe_vk_descriptor_set_layout,
                               &main_render_target);
  pe_vk_descriptor_update(model, &main_render_target);
  if (model->parts.count > 0)
    pe_vk_model_parts_create_descriptors(model);

  //init model matrix
  glm_mat4_identity(model->model_mat);
  //setup Uniform Buffer Object
  glm_mat4_copy(model->model_mat,model->uniform_buffer_object.model);
  glm_mat4_copy(main_camera.projection, model->uniform_buffer_object.projection);
  glm_mat4_copy(main_camera.view, model->uniform_buffer_object.view);

  return model;
}

//INFO reuses pe_vk_load_model() as-is for the mesh, vertex/index buffers and
//uniform buffer - current_skin just gives pe_loader_model_from_memory()
//somewhere to put the skin and animations it finds along the way. what this
//does not yet do: model's descriptor sets and pool here are still the plain
//(uniform buffer only) ones pe_vk_load_model() always makes, not the
//uniform+texture+joint matrices SSBO layout skinned.vert needs - drawing a
//skin still wants a pipeline, descriptor layout and per frame joint matrix
//upload of its own, none of which exist yet
PModel *pe_vk_load_skin(PSkin *skin, PModel *model, const char *path) {
  ZERO(*skin);
  skin->mesh = model;

  current_skin = skin;
  pe_vk_load_model(model, path);
  current_skin = NULL;

  return model;
}

//INFO joints and animation channels are copied by value, one array_add() per
//element, into freshly sized arrays - never memcpy'd wholesale, because a
//channel's node pointer and a joint's parent pointer point into source's
//joints array and have to be remapped into skin's own. Node.id (set to its
//index by pe_load_skin()) is what makes that remap an array_get(), not a
//search: skin->joints keeps the same order and indices as source->joints, so
//source_pointer->id is skin's index for the same joint.
//
//A channel's sampler keyframes (inputs/outputs) are not copied - shared by
//pointer with source instead, since play_animation() only ever reads them; a
//joint's translation/rotation and an animation's time/loop are the only
//state playback writes, and those live in the arrays this does duplicate.
void pe_vk_skin_instance(PSkin *skin, PSkin *source) {
  memcpy(skin, source, sizeof(PSkin));

  array_init(&skin->joints, sizeof(Node), source->joints.count);
  for (int i = 0; i < source->joints.count; i++)
    array_add(&skin->joints, array_get(&source->joints, i));
  for (int i = 0; i < skin->joints.count; i++) {
    Node *joint = array_get(&skin->joints, i);
    if (joint->parent != NULL)
      joint->parent = array_get(&skin->joints, joint->parent->id);
  }

  array_init(&skin->animations, sizeof(PAnimation), source->animations.count);
  for (int a = 0; a < source->animations.count; a++) {
    PAnimation *source_animation = array_get(&source->animations, a);
    PAnimation animation = *source_animation;

    array_init(&animation.channels, sizeof(PAnimationChannel),
              source_animation->channels.count);
    for (int c = 0; c < source_animation->channels.count; c++) {
      PAnimationChannel *source_channel =
          array_get(&source_animation->channels, c);
      PAnimationChannel channel = *source_channel;
      channel.node = array_get(&skin->joints, source_channel->node->id);
      array_add(&animation.channels, &channel);
    }
    array_add(&skin->animations, &animation);
  }

  ZERO(skin->shader_storage_buffers);
  ZERO(skin->shader_storage_buffers_memory);
}

//INFO one more thing to draw with geometry that is already on the gpu. the
//vertex and index buffers are shared with source, but the copy gets its own
//uniform buffers and descriptor sets, because those carry the per instance
//model matrix. sharing them would make every copy sit where the last one moved
PModel *pe_vk_model_instance(PModel *model, PModel *source) {

  memcpy(model, source, sizeof(PModel));

  ZERO(model->uniform_buffers);
  ZERO(model->uniform_buffers_memory);
  ZERO(model->descriptor_sets);
  ZERO(model->part_descriptor_sets);

  pe_vk_create_uniform_buffers(model, &main_render_target);
  pe_vk_descriptor_pool_create(model, &main_render_target);
  pe_vk_create_descriptor_sets(model, pe_vk_descriptor_set_layout,
                               &main_render_target);
  pe_vk_descriptor_update(model, &main_render_target);
  if (model->parts.count > 0)
    pe_vk_model_parts_create_descriptors(model);

  glm_mat4_identity(model->model_mat);
  glm_mat4_copy(model->model_mat, model->uniform_buffer_object.model);

  return model;
}

//same as pe_vk_model_instance() above, but against the textured descriptor
//set layout, and actually writing the copy's own binding 1 to source's
//texture - pe_vk_descriptor_update() (what the plain path calls) only ever
//touches binding 0, so a copy sharing a textured source's look needs
//pe_vk_descriptor_with_image_update() instead, or its sampler binding is
//left unwritten (read: garbage) same as the plain layout would leave it
PModel *pe_vk_model_instance_textured(PModel *model, PModel *source) {

  memcpy(model, source, sizeof(PModel));

  ZERO(model->uniform_buffers);
  ZERO(model->uniform_buffers_memory);
  ZERO(model->descriptor_sets);

  pe_vk_create_uniform_buffers(model, &main_render_target);
  pe_vk_descriptor_pool_create(model, &main_render_target);
  pe_vk_create_descriptor_sets(model, pe_vk_descriptor_set_layout_with_texture,
                               &main_render_target);
  pe_vk_descriptor_with_image_update(model, &main_render_target);

  glm_mat4_identity(model->model_mat);
  glm_mat4_copy(model->model_mat, model->uniform_buffer_object.model);

  return model;
}

//same as pe_vk_model_instance_textured() above, but against the skinned
//descriptor set layout (uniform + texture + joint-matrix storage buffer at
//binding 2), for a source loaded with pe_vk_load_skin() rather than
//pe_vk_load_model(). skin is shared, not copied: every instance made from
//the same skin binds the very same storage buffer, so they all draw
//whatever single pose that skin is currently posed to - there is no
//per-instance animation state yet, only per-instance transform/texture
PModel *pe_vk_model_instance_skinned(PModel *model, PModel *source,
                                     struct PSkin *skin) {

  memcpy(model, source, sizeof(PModel));

  ZERO(model->uniform_buffers);
  ZERO(model->uniform_buffers_memory);
  ZERO(model->descriptor_sets);

  pe_vk_create_uniform_buffers(model, &main_render_target);
  pe_vk_descriptor_pool_create(model, &main_render_target);
  pe_vk_create_descriptor_sets(model, pe_vk_descriptor_set_layout_skinned,
                               &main_render_target);
  pe_vk_descriptor_skinned_update(model, skin, &main_render_target);

  glm_mat4_identity(model->model_mat);
  glm_mat4_copy(model->model_mat, model->uniform_buffer_object.model);

  return model;
}

void pe_model_transform_reset(PModel *model) {
  glm_mat4_identity(model->model_mat);
  glm_vec3_zero(model->position);
}

void pe_model_transform(PModel *model, vec3 position, float angle, vec3 axis,
                        vec3 scale) {
  glm_mat4_identity(model->model_mat);
  glm_translate(model->model_mat, position);
  glm_rotate(model->model_mat, glm_rad(angle), axis);
  glm_scale(model->model_mat, scale);

  glm_vec3_copy(position, model->position);
}

//INFO the translation lives in the fourth column, so it can be replaced
//without touching the rotation and scale in the upper 3x3 - composing a fresh
//translate onto the matrix instead would move it by position every call
void pe_model_set_position(PModel *model, vec3 position) {
  glm_vec3_copy(position, model->model_mat[3]);
  glm_vec3_copy(position, model->position);
}

void pe_model_translate(PModel *model, vec3 offset) {
  glm_translate(model->model_mat, offset);
  glm_vec3_copy(model->model_mat[3], model->position);
}

void pe_model_rotate(PModel *model, float angle, vec3 axis) {
  glm_rotate(model->model_mat, glm_rad(angle), axis);
}

void pe_model_scale(PModel *model, vec3 scale) {
  glm_scale(model->model_mat, scale);
}

void pe_clean_model(PModel* model){
  for(int i = 0; i < model->uniform_buffers_memory.count; i++){
    VkDeviceMemory* memory = array_get(&model->uniform_buffers_memory, i);
    //printf("Freeying uniform buffermemory %p\n", *memory);
    vkFreeMemory(vk_device, *memory, NULL);
  }

  vkDestroyDescriptorPool(vk_device, model->descriptor_pool, NULL);
  if (model->parts.count > 0)
    vkDestroyDescriptorPool(vk_device, model->part_descriptor_pool, NULL);
  if (model->has_extra_texture) {
    vkDestroyDescriptorPool(vk_device, model->extra_descriptor_pool, NULL);
    pe_vk_clean_image(&model->extra_texture);
  }

  pe_vk_clean_shader(&model->shader);

  vkFreeMemory(vk_device,model->index_buffer.memory, NULL); 
  vkFreeMemory(vk_device,model->vertex_buffer.memory, NULL); 

}

int pe_load_model_path(PModel* model, const char *path) {
  File new_file;

  if (load_file(path, &new_file) == -1) {
    LOG("**** load_file() error\n");
    return -1;
  }

  cgltf_result result =
      pe_loader_model_from_memory(model, new_file.data, new_file.size_in_bytes, path);

  if (result != cgltf_result_success) {
    LOG("Model no loaded: %s \n", new_file.path);
    if (result == cgltf_result_io_error) {
      LOG("Buffer no loaded: %s \n", new_file.path);
      LOG("IO ERROR\n");
    }
    return -1;
  }

  close_file(&new_file);

  // LOG("glTF2 loaded: %s. \n",path);

  return 1;//TODO
}
