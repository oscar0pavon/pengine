
#include "text_renderer.h"
#include "engine.h"

#include <freetype/freetype.h>
#include <ft2build.h>
#include <engine/files.h>

FT_GlyphSlot glyph;

DrawData text_draw_data;

void set_text_size(float size) { pe_text_set_size(size); }

void pe_text_set_size(int size) { FT_Set_Pixel_Sizes(face, 0, size); }

void text_texture_create_buffer() {

  glGenTextures(1, &text_draw_data.texture);
  glBindTexture(GL_TEXTURE_2D, text_draw_data.texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

void text_render_with_data(const char *text, float x, float y,
                           float pixel_size_x, float pixel_size_y, vec4 color) {
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_CULL_FACE);

  glUseProgram(text_draw_data.shader);

  GLint uniform_color = get_uniform_location(text_draw_data.shader, "color");

  const char *p;

  for (p = text; *p; p++) {
    if (FT_Load_Char(face, *p, FT_LOAD_RENDER))
      continue;

    glBindTexture(GL_TEXTURE_2D, text_draw_data.texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, glyph->bitmap.width,
                 glyph->bitmap.rows, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                 glyph->bitmap.buffer);

    check_error("text texture");

    float x2 = x + glyph->bitmap_left * pixel_size_x;
    float y2 = -y - glyph->bitmap_top * pixel_size_y;
    float w = glyph->bitmap.width * pixel_size_x;
    float h = glyph->bitmap.rows * pixel_size_y;

    GLfloat box[4][4] = {
        {x2, -y2, 0, 0},
        {x2 + w, -y2, 1, 0},
        {x2, -y2 - h, 0, 1},
        {x2 + w, -y2 - h, 1, 1},
    };

    glUniform4fv(uniform_color, 1, color);
    check_error("send text color");

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, text_draw_data.vertex);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glBufferData(GL_ARRAY_BUFFER, sizeof box, box, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    x += (glyph->advance.x / 64) * pixel_size_x;
    y += (glyph->advance.y / 64) * pixel_size_y;

    check_error("texture error");
  }
  glEnable(GL_CULL_FACE);
}
void text_render(const char *text, float x, float y, float pixel_size_x,
                 float pixel_size_y, bool mark) {
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_CULL_FACE);

  glUseProgram(text_draw_data.shader);

  GLint uniform_color = get_uniform_location(text_draw_data.shader, "color");

  const char *p;

  for (p = text; *p; p++) {
    if (FT_Load_Char(face, *p, FT_LOAD_RENDER))
      continue;

    glBindTexture(GL_TEXTURE_2D, text_draw_data.texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, glyph->bitmap.width,
                 glyph->bitmap.rows, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                 glyph->bitmap.buffer);

    check_error("text texture");

    float x2 = x + glyph->bitmap_left * pixel_size_x;
    float y2 = -y - glyph->bitmap_top * pixel_size_y;
    float w = glyph->bitmap.width * pixel_size_x;
    float h = glyph->bitmap.rows * pixel_size_y;

    GLfloat box[4][4] = {
        {x2, -y2, 0, 0},
        {x2 + w, -y2, 1, 0},
        {x2, -y2 - h, 0, 1},
        {x2 + w, -y2 - h, 1, 1},
    };
    if (mark)
      glUniform4fv(uniform_color, 1, (vec4){1, 0, 1, 1});
    else
      glUniform4fv(uniform_color, 1, (vec4){1, 1, 1, 1});

    check_error("send text color");

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, text_draw_data.vertex);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glBufferData(GL_ARRAY_BUFFER, sizeof box, box, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    x += (glyph->advance.x / 64) * pixel_size_x;
    y += (glyph->advance.y / 64) * pixel_size_y;

    // check_error("texture error");
  }
  glEnable(GL_CULL_FACE);
}

void text_shaders_init() {
  GLuint text_fragment_shader;
  GLuint text_vertex_shader;

  text_fragment_shader =
      compile_shader(pe_shader_src_std_frag, GL_FRAGMENT_SHADER);
  text_vertex_shader =
      compile_shader(text_vertex_shader_source, GL_VERTEX_SHADER);

  text_draw_data.shader =
      create_engine_shader(text_vertex_shader, text_fragment_shader);
}

void text_renderer_init() {
  FT_Library ft;

  if (FT_Init_FreeType(&ft)) {
    LOG("Could not init freetype library\n");
    return;
  }

  File font;
  load_file(file_font, &font);
  int error = FT_New_Memory_Face(ft, font.data, font.size_in_bytes, 0, &face);

  if (error != 0) {
    LOG("Could not open font\n");
    return;
  }

  glyph = face->glyph;

  text_shaders_init();
  text_texture_create_buffer();
  glGenBuffers(1, &text_draw_data.vertex);

  pixel_size_x = 2.0 / camera_width_screen;
  pixel_size_y = 2.0 / camera_heigth_screen;
}

void text_renderer_update_pixel_size() {
  pixel_size_x = 2.0 / camera_width_screen;
  pixel_size_y = 2.0 / camera_heigth_screen;
}

void text_render_in_screen_space_with_data(const char *text,
                                           TextRenderData *data) {
  float text_position_x =
      (-(camera_width_screen / 2) + data->position[0]) * pixel_size_x;
  float text_position_y =
      (((camera_heigth_screen / 2) - data->size) + data->position[1]) *
      pixel_size_y;

  FT_Set_Pixel_Sizes(face, 0, data->size);
  text_render_with_data(text, text_position_x, text_position_y, pixel_size_x,
                        pixel_size_y, data->color);
}

void text_render_in_screen_space(int text_size, const char *text, int x,
                                 int y) {
  float text_position_x = (-(camera_width_screen / 2) + x) * pixel_size_x;
  float text_position_y =
      (((camera_heigth_screen / 2) - text_size) + y) * pixel_size_y;

  FT_Set_Pixel_Sizes(face, 0, text_size);
  text_render(text, text_position_x, text_position_y, pixel_size_x,
              pixel_size_y, false);
}
