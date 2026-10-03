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
#define STBI_ONLY_HDR
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

static int pe_texture_upload_format(PTexture* texture, PImage* image,
                                    VkFormat format){

    pe_vk_create_texture_from_image_format(texture, image, format, true);
    texture->gpu_loaded = true;
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

//the half precision float nearest to value, which is what a high dynamic range
//texture holds: the sun in a panorama is thousands of times brighter than the
//sky round it, and 8 bits cannot keep both
static u16 float_to_half(float value) {
    if(value > 65504.0f) value = 65504.0f;
    if(value < 0.0f) value = 0.0f;

    u32 bits;
    memcpy(&bits, &value, sizeof(bits));
    int exponent = (int)((bits >> 23) & 0xFF) - 127 + 15;
    u32 mantissa = bits & 0x7FFFFF;

    if(exponent <= 0) return 0;
    return (u16)((exponent << 10) | ((mantissa + 0x1000) >> 13));
}

//a Radiance .hdr, kept as light and not as colour: no sRGB curve, no clamping
//to 1. false if the file is not one
bool pe_load_hdr_image(const char* path, PHdrImage* image){

    int channels;
    image->pixels = stbi_loadf(path, &image->width, &image->height, &channels, 3);
    if(image->pixels == NULL){
        LOG("Image not decoded: %s\n", path);
        return false;
    }
    return true;
}

void pe_free_hdr_image(PHdrImage* image){
    stbi_image_free(image->pixels);
    image->pixels = NULL;
}

void pe_texture_from_hdr_image(PTexture* texture, const PHdrImage* hdr){

    u16* halves = malloc((size_t)hdr->width * hdr->height * 4 * sizeof(u16));
    for(size_t i = 0; i < (size_t)hdr->width * hdr->height; i++){
        for(int c = 0; c < 3; c++)
            halves[i * 4 + c] = float_to_half(hdr->pixels[i * 3 + c]);
        halves[i * 4 + 3] = float_to_half(1.0f);
    }

    PImage image;
    ZERO(image);
    image.width = hdr->width;
    image.heigth = hdr->height;
    image.pixels_data = (unsigned char*)halves;

    texture->width = hdr->width;
    texture->heigth = hdr->height;
    pe_vk_create_texture_from_image_format(texture, &image,
                                           VK_FORMAT_R16G16B16A16_SFLOAT, true);
    texture->gpu_loaded = true;
    free_image(&image);
}

bool pe_load_hdr_texture(const char* path, PTexture* texture){

    PHdrImage image;
    if(!pe_load_hdr_image(path, &image))
        return false;

    pe_texture_from_hdr_image(texture, &image);
    pe_free_hdr_image(&image);
    return true;
}

//the normal of a surface that is flat, which a part with no normal map is
//drawn with
int pe_texture_flat_normal(PTexture* texture){

    PImage image;
    ZERO(image);
    image.width = 1;
    image.heigth = 1;
    image.pixels_data = malloc(4);
    image.pixels_data[0] = 128;
    image.pixels_data[1] = 128;
    image.pixels_data[2] = 255;
    image.pixels_data[3] = 255;

    texture->width = 1;
    texture->heigth = 1;
    int result = pe_texture_upload_format(texture, &image, VK_FORMAT_R8G8B8A8_UNORM);
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

//data that is not colour, a normal map or the metal and roughness of a
//surface, has to be read as it is, with no sRGB curve
int texture_load_from_memory_linear(PTexture* texture, u32 size, void* data){

    PImage image;
    ZERO(image);

    if(image_load_from_memory(&image, data, size) == -1)
        return -1;

    texture->width = image.width;
    texture->heigth = image.heigth;

    int result = pe_texture_upload_format(texture, &image, VK_FORMAT_R8G8B8A8_UNORM);

    free_image(&image);

    return result;
}

void free_image(PImage* image){
  free(image->pixels_data);
}
