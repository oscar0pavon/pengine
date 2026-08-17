
#ifndef VK_UNIFORM
#define VK_UNIFORM

#include "vk_buffer.h"

#include <engine/components/skinned_mesh_component.h>
#include <engine/model.h>

PBufferCreateInfo buffer_color;

void pe_vk_uniform_buffer_update(uint32_t image_index);

void pe_vk_create_uniform_buffers(PModel *model);
void pe_vk_uniform_buffer_update_one(uint32_t image_index);
void pe_vk_uniform_buffer_update_two(uint32_t image_index);
void pe_vk_uniform_buffer_update_skeletal(uint32_t image_index);

void pe_vk_create_shader_storage_buffers(PSkinnedMeshComponent *skinned);

#endif
