#ifndef TEXTURES_RENDER_H
#define TEXTURES_RENDER_H

#include <engine/model.h>

typedef struct{
    unsigned char* data;
    u8 size;
}TextureCreated;

void init_UV_draw(PModel* model);

void draw_UV();

void merge_textures(const char* name);

void render_to_texture(int size, void(*function)(int) );

void init_model_to_draw_texture();

void scale_UV(float size, PModel* model, vec2 UV_offset);

void translate_UV(vec3 tranlation, PModel* model, vec2 UV_offset);

void texture_export(int size);

TextureCreated texture_create_to_memory(u8 format, u8 size);

const char* texture_current_export_name;

#endif // !TEXTURES_RENDER_H
