#ifndef PE_VK_LOGICAL_H
#define PE_VK_LOGICAL_H


//how many texels along a sampler may be read to filter a surface seen at a
//grazing angle, 1 for none
extern float pe_vk_max_anisotropy;

int pe_vk_create_logical_device();

#endif
