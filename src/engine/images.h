#ifndef IMAGES_H
#define IMAGES_H


#include "numbers.h"
#include <stdbool.h>
#include <stdint.h>
#include "renderer/vulkan.h"

typedef struct PImage{
  unsigned short int width;
  unsigned short int heigth;
  unsigned char* pixels_data;
}PImage;

typedef struct PTexture{
    unsigned int id;
		int format;
		bool gpu_loaded;
    VkImage image;
    VkDeviceMemory memory;
    u32 mip_level;
    VkSampler sampler;
    VkImageView image_view;
    int memory_file_descriptor;
    uint32_t width;
    uint32_t heigth;
}PTexture;

int pe_load_image(const char* path, PImage* image);
void free_image(PImage*);

int pe_load_texture(const char* path, PTexture*);

int texture_load_from_memory(PTexture* texture,u32 size,void* data);

int image_load_from_memory(PImage* image,void* data, u32 size);

void pe_gpu_load_texture(PTexture* texture);

#endif
