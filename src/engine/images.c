#include "images.h"
#include <vulkan/vulkan.h>
#include <engine/macros.h>
#include <engine/file_loader.h>
#include <engine/log.h>
#include <engine/renderer/vk_images.h>
#include <engine/window_manager.h>
#include <lodepng.h>


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

int image_load_from_memory(PImage* image, void* data, u32 size){

    unsigned int width, height;

    unsigned char* image_data = NULL;

    unsigned int error = lodepng_decode32(&image_data,&width,&height,data,size);
    if(error){
        LOG("Image not decoded from memory (%s)\n", lodepng_error_text(error));
        return -1;
    }

    image->heigth = (unsigned short)height;
    image->width = (unsigned short)width;
    image->pixels_data = image_data;
    return 0;
}

//INFO only the vulkan backend still owns pixels. pe_tex_to_gpu is an ES2
//leftover that free()s the image and uploads nothing, so a texture asked for
//under PEWMOPENGLES2 is reported rather than silently handed back empty
static int pe_texture_upload(PTexture* texture, PImage* image){

    if(pe_renderer_type == PEWMVULKAN){
        pe_vk_create_texture_from_image(texture, image);
        texture->gpu_loaded = true;
        return 0;
    }

    LOG("Texture upload not implemented for this renderer\n");
    return -1;
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
