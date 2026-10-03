#include "images.h"
#include <vulkan/vulkan.h>
#include <engine/macros.h>
#include <engine/file_loader.h>
#include <engine/log.h>
#include <engine/renderer/vk_images.h>
#include <engine/window_manager.h>
#include <lodepng.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#include <ThirdParty/stb_image.h>


int pe_load_image(const char* path,  PImage* out_image){

    PImage new_image;

    unsigned int width, height;

    unsigned char* image_data = NULL;
 
    unsigned int error = lodepng_decode32_file(&image_data,&width,&height,path);
    if(error){
        LOG("Image not decoded: %s (%s)\n", path, lodepng_error_text(error));
        return -1;
    }

    new_image.heigth = (unsigned short)height;
    new_image.width = (unsigned short)width;
    new_image.pixels_data = image_data;
    memcpy(out_image,&new_image,sizeof(PImage));
    return 0;
}

//INFO lodepng reads only png. a model's textures come out of its glb as they
//were authored, and a jpeg is the common other one
int image_load_from_memory(PImage* image, void* data, u32 size){

    unsigned int width, height;

    unsigned char* image_data = NULL;

    unsigned int error = lodepng_decode32(&image_data,&width,&height,data,size);
    if(error){
        int jpeg_width, jpeg_height, channels;
        image_data = stbi_load_from_memory(data, size, &jpeg_width, &jpeg_height,
                                           &channels, 4);
        if(image_data == NULL){
            LOG("Image not decoded from memory (%s)\n", lodepng_error_text(error));
            return -1;
        }
        width = jpeg_width;
        height = jpeg_height;
    }

    image->heigth = (unsigned short)height;
    image->width = (unsigned short)width;
    image->pixels_data = image_data;
    return 0;
}

static int pe_texture_upload(PTexture* texture, PImage* image){

    pe_vk_create_texture_from_image(texture, image);
    texture->gpu_loaded = true;
    return 0;
}

int pe_load_texture(const char* path, PTexture* new_texture){

    PImage image;
    ZERO(image);

    if(pe_load_image(path, &image) == -1){
        new_texture->id = -1;
        return -1;
    }

    new_texture->width = image.width;
    new_texture->heigth = image.heigth;

    int result = pe_texture_upload(new_texture, &image);

    free_image(&image);

    if(result == -1)
        return -1;

    return 1;
}

//one white pixel, which a part with no texture of its own is drawn with so its
//colour alone shows
int pe_texture_white(PTexture* texture){

    PImage image;
    ZERO(image);
    image.width = 1;
    image.heigth = 1;
    image.pixels_data = malloc(4);
    memset(image.pixels_data, 255, 4);

    texture->width = 1;
    texture->heigth = 1;
    int result = pe_texture_upload(texture, &image);
    free_image(&image);
    return result;
}

int texture_load_from_memory(PTexture* texture, u32 size, void* data){

    PImage image;
    ZERO(image);

    if(image_load_from_memory(&image, data, size) == -1)
        return -1;

    texture->width = image.width;
    texture->heigth = image.heigth;

    int result = pe_texture_upload(texture, &image);

    free_image(&image);

    return result;
}

void free_image(PImage* image){
  free(image->pixels_data);
}
