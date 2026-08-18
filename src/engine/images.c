#include "images.h"
#include <vulkan/vulkan.h>
#include <engine/macros.h>
#include <engine/file_loader.h>
#include <engine/log.h>
#include <lodepng.h>


int pe_load_image(const char* path,  PImage* out_image){

    PImage new_image;

    unsigned int width, height;

    unsigned char* image_data = NULL;
 
    unsigned int error = lodepng_decode32_file(&image_data,&width,&height,path);

    new_image.heigth = (unsigned short)height;
    new_image.width = (unsigned short)width;
    new_image.pixels_data = image_data;
    memcpy(out_image,&new_image,sizeof(PImage));
    return 0;
}



void free_image(PImage* image){
  free(image->pixels_data);
}
